#define QOP_CLOVER_DEFAULT_PRECISION 'F'
#include <clover.h>
#undef QOP_CLOVER_DEFAULT_PRECISION
#define QOP_CLOVER_DEFAULT_PRECISION 'D'
#include <clover.h>

/* Solve
 *   D_cl psi = eta
 *
 * with psi_0 as an initial guess
 *
 */ 

#define MAX_OPTIONS (Q(LOG_CG_RESIDUAL)     | \
                     Q(LOG_TRUE_RESIDUAL)   | \
                     Q(FINAL_CG_RESIDUAL)   | \
                     Q(FINAL_DIRAC_RESIDUAL))

static void *
allocate_fgauge(struct Q(State) *state,
                struct QF(Gauge) *fg,
                const struct QD(Gauge) *dg)
{
    void *pgf;
    void *ptr;
    int  u_s, ce_s, co_s;
    size_t size;

    fg->state = state;
    fg->size = 0;
    u_s = qf(sizeof_gauge)(state->volume);
    ce_s = qf(sizeof_clover)(state->even.full_size);
    co_s = qf(sizeof_clover)(state->odd.full_size);
    pgf = q(allocate_aligned)(state, &size, &ptr, 0, u_s + ce_s + 2 * co_s);
    if (pgf == NULL)
        return NULL;

    fg->size = size;
    fg->g_data = ptr;
    fg->ce_data = (void *)(((char *)(fg->g_data)) + u_s);
    fg->co_data = (void *)(((char *)(fg->ce_data)) + ce_s);
    fg->cox_data = (void *)(((char *)(fg->co_data)) + co_s);

    q(g_f_eq_d)(fg->g_data, Q(DIM) * state->volume, dg->g_data);
    q(c_f_eq_d)(fg->ce_data, state->even.full_size, dg->ce_data);
    q(c_f_eq_d)(fg->co_data, state->even.full_size, dg->co_data);
    q(c_f_eq_d)(fg->cox_data, state->even.full_size, dg->cox_data);

    return pgf;
}

int
Q(mixed_D_CG)(struct QD(Fermion)          *psi,
              int                         *out_iterations,
              double                      *out_epsilon,
              const struct QD(Fermion)    *psi_0,
              const struct QD(Gauge)      *gauge,
              const struct QD(Fermion)    *eta,
              int                          f_iter,
              int                          max_iterations,
              double                       min_epsilon,
              unsigned int                 options)
{
    DECLARE_STATE;
    long long flops = 0;
    long long sent = 0;
    long long received = 0;
    void *ptr_d = 0;
    void *ptr_f = 0;
    void *ptr_g = 0;
    size_t ptr_size_d = 0;
    size_t ptr_size_f = 0;
    void *temps = 0;
    int status = 1;
    struct FermionD *chi_e = 0;
    struct FermionD *t0_e = 0;
    struct FermionD *t1_e = 0;
    struct FermionD *t0_o = 0;
    
    struct QF(Gauge) gauge_F;
    struct FermionF *delta_Fe = 0;
    struct FermionF *dx_Fe = 0;
    struct FermionF *zero_Fe = 0;
    struct FermionF *pi_Fe = 0;
    struct FermionF *rho_Fe = 0;
    struct FermionF *zeta_Fe = 0;
    struct FermionF *t0_Fe = 0;
    struct FermionF *t1_Fe = 0;
    struct FermionF *t0_Fo = 0;
    struct FermionF *t1_Fo = 0;

    double dirac_residual = 0;
    double rhs_norm = 0;
    double scaled_eps;
    int iter_left;
    char *cg_error = 0;
#define CG_ERROR(msg) do { cg_error = msg; goto end; } while (0)
#define CG_ERROR_T(msg) do { \
        END_TIMING(state, flops, sent, received); \
        cg_error = msg; goto end; } while (0)

    /* check arguments */
    CHECK_ARG0(psi);
    CHECK_ARGn(psi_0, "mD_CG");
    CHECK_ARGn(gauge, "mD_CG");
    CHECK_ARGn(eta, "mD_CG");

    /* clear bits we do not understand */
    options = options & MAX_OPTIONS;

    /* allocate double locals */
    ptr_d = qd(allocate_eo)(state, &ptr_size_d, &temps,
                            0,  /* header */
                            3,  /* evens */
                            1); /* odds */
    if (ptr_d == 0)
        CG_ERROR("mixed_D_CG(): not enough memory");

    chi_e = temps;
    t0_e  = temps = qd(step_even)(state, temps);
    t1_e  = temps = qd(step_even)(state, temps);
    t0_o  = temps = qd(step_even)(state, temps);

    /* allocate float temps */
    ptr_f = qf(allocate_eo)(state, &ptr_size_f, &temps,
                            0,  /* header */
                            8,  /* evens */
                            2); /* odds */
    if (ptr_f == 0)
        CG_ERROR("mixed_D_CG(): not enough memory");

    delta_Fe = temps;
    dx_Fe    = temps = qf(step_even)(state, temps);
    zero_Fe  = temps = qf(step_even)(state, temps);
    rho_Fe   = temps = qf(step_even)(state, temps);
    pi_Fe    = temps = qf(step_even)(state, temps);
    zeta_Fe  = temps = qf(step_even)(state, temps);
    t0_Fe    = temps = qf(step_even)(state, temps);
    t1_Fe    = temps = qf(step_even)(state, temps);
    t0_Fo    = temps = qf(step_even)(state, temps);
    t1_Fo    = temps = qf(step_odd)(state, temps);

    ptr_g = allocate_fgauge(state, &gauge_F, gauge);
    if (ptr_g == 0)
        CG_ERROR("mixed_D_CG(): not enough memory");

    BEGIN_TIMING(state);
    /* compute the norm of the RHS */
    flops += qd(op_norm2)(&rhs_norm, eta, state);
    
    if (options) {
        qd(zprint)(state, "mDCL CG", "rhs norm %e normalized epsilon %e",
                   rhs_norm, min_epsilon * rhs_norm);
    }

    /* setup communication */
    if (q(setup_comm)(state, sizeof (double)))
        CG_ERROR_T("DDW_CG(): communication setup failed");
        
    /* precondition */
    qd(cg_precondition)(chi_e, state,
                        gauge,
                        eta->even, eta->odd,
                        &flops, &sent, &received,
                        t0_e,
                        t0_o);
    qd(f_copy)(psi->even,
               state->even.full_size,
               psi_0->even);

    qf(f_zero)(zero_Fe, state->even.full_size);

    scaled_eps = min_epsilon * rhs_norm;
    for (iter_left = max_iterations; iter_left > 0 && status;) {
        int here_iter;

        qd(op_even_M)(t0_e, state, gauge, psi->even,
                      &flops, &sent, &received, t0_o);
        qd(op_even_Mx)(t1_e, state, gauge, t0_e,
                       &flops, &sent, &received, t0_o);
        q(f_f_eq_dmd)(delta_Fe, state->even.full_size, chi_e, t1_e);
        
        /* run the solver for a while */
        if (q(setup_comm)(state, sizeof (float))) {
            CG_ERROR_T("DDW_CG(): communication setup failed");
        }
        status = qf(cg_solver)(dx_Fe, "MDCL CG", &here_iter, out_epsilon,
                               state, &gauge_F,
                               zero_Fe, delta_Fe, NULL, NULL,
                               iter_left > f_iter ? f_iter : iter_left,
                               scaled_eps, options,
                               &flops, &sent, &received,
                               rho_Fe, pi_Fe, zeta_Fe, t0_Fe,
                               t1_Fe, t0_Fo, t1_Fo);

        if (q(setup_comm)(state, sizeof (double))) {
            CG_ERROR_T("DDW_CG(): communication setup failed");
        }
        flops += q(f_d_eq_dpf)(psi->even, state->even.full_size,
                               psi->even, dx_Fe);
        iter_left -= here_iter;
        *out_iterations = max_iterations - iter_left;
        /* continue or not */
        if (status > 1)
            CG_ERROR_T(NULL);
    }

    /* inflate */
    qd(cg_inflate)(psi->odd,
                   state, gauge, eta->odd, psi->even,
                   &flops, &sent, &received,
                   t0_o);    

    if (options & (Q(FINAL_DIRAC_RESIDUAL) | Q(LOG_DIRAC_RESIDUAL))) {
        dirac_residual = qd(cg_dirac_error)(psi->even, psi->odd,
                                            state, gauge,
                                            eta->even, eta->odd,
                                            &flops, &sent, &received,
                                            t0_e, t0_o);
    }

    END_TIMING(state, flops, sent, received);

    /* output final residuals if desired */
        if (options) {
        qx(zprint)(state, "DCL CG", "status %d, total iterations %d",
                  status, *out_iterations);
    }
    if (options & (Q(FINAL_CG_RESIDUAL) | Q(LOG_CG_RESIDUAL))) {
        double norm = rhs_norm == 0? 1: rhs_norm;

        qx(zprint)(state, "DCL CG", "solver residual %e normalized %e",
                   *out_epsilon, *out_epsilon / norm);
    }
    if (options & (Q(FINAL_DIRAC_RESIDUAL) | Q(LOG_DIRAC_RESIDUAL))) {
        double norm = rhs_norm == 0? 1: rhs_norm;

        qx(zprint)(state, "DCL CG", "Dirac residual %e normalized %e",
                   dirac_residual, dirac_residual / norm);
    }
    if (rhs_norm != 0.0)
        *out_epsilon = *out_epsilon / rhs_norm;
        
end:    
    if (ptr_d)
        q(free)(state, ptr_d, ptr_size_d);
    if (ptr_f)
        q(free)(state, ptr_f, ptr_size_f);
    if (ptr_g)
        q(free)(state, ptr_g, gauge_F.size);
    if (cg_error)
        return q(set_error)(state, 0, cg_error);
    if (status != 0)
        q(set_error)(state, 0, "mixed_D_CG() solver failed to converge");
    return status;
}


