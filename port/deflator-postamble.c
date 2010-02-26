#include <clover.h>
#include <math.h>

#if defined(HAVE_LAPACK)
#  include <lapack.h>
#elif defined(HAVE_GSL)
#  include <gsl/gsl_linalg.h>
#else
#  error "no linear algebra library"
#endif

int
q(df_postamble)(
        struct Q(State)         *s,
        struct Q(Deflator)      *d)
{
    int i;

    assert(NULL != s &&
            NULL != d);
    if (NULL == s ||
            NULL == d ||
            d->frozen || 
            d->umax <= d->usize ||
            d->vsize < d->nev)
        return 0;

#define eps_reortho 1e-7
#define n_reortho   2
    double v_norm2_min = eps_reortho * eps_reortho;
    int unew = 0,
        i_v = 0;
    long int usize_old = d->usize;
    while ((d->usize < d->umax) && (i_v < d->nev)) {
        /* reorthogonalize n_reortho times */
/* macros to reuse workspace */
#define cur_v       (d->work_c_1)
#define cur_Av      (d->work_c_2)
#define cur_aux     (d->work_c_3)
        q(latmat_c_get_col)(d->V, i_v, cur_v);
        
        if (0 < d->usize) {
            int i_reortho;
            for (i_reortho = n_reortho; i_reortho--; ) {
                latmat_c cur_U = q(latmat_c_submat_col)(d->U, 0, d->usize);
                q(lat_lmH_dot_lv)(d->usize, 
                               cur_U, 
                               cur_v, 
                               d->zwork);
                q(lat_lm_dot_zv)(d->usize, 
                              cur_U, 
                              d->zwork, 
                              cur_Av);
                /* FIXME optimize with a special primitive 
                   cur_v <- cur_v - cur_Av */
                q(lat_c_axpy_d)(-1., cur_Av, cur_v);
            }
        }
        double v_norm2 = q(lat_c_nrm2)(cur_v);
        if (v_norm2 < v_norm2_min) {    /* skip */
            i_v++;
            continue;
        }
        q(lat_c_scal_d)(1 / sqrt(v_norm2), cur_v);
        q(latmat_c_insert_col)(d->U, d->usize, cur_v);

        /*  XXX requires(?) double precision operator */
//        latvec_cz_copy(cur_v, cur_z_v);
        q(latvec_c_linop)(s, cur_Av, cur_v, cur_aux);
        latmat_c cur_U = q(latmat_c_submat_col)(d->U, 0, d->usize + 1);
        q(lat_lmH_dot_lv)(d->usize + 1, cur_U, cur_Av, d->H + d->usize * d->umax);
        for (i = 0; i < d->usize; i++) {
            doublecomplex *p1 = d->H + i + d->usize * d->umax,
                          *p2 = d->H + d->usize + i * d->umax;
            p2->r =  p1->r;
            p2->i = -p1->i;
        }
        d->H[d->usize * (d->umax + 1)].i = 0.;

        d->usize ++;
        unew ++;
        i_v ++;
    }
    assert(usize_old + unew == d->usize);

    /* compute Cholesky decomposition */
    memcpy(d->C, d->H, d->usize * d->umax * sizeof(d->C[0]));

#if HAVE_LAPACK
    long int usize  = d->usize,
             umax   = d->umax,
             info   = 0;
    char cU = 'U';
    zpotrf_(&cU, &usize, d->C, &umax, &info, 1);
    assert(0 == info);
#elif HAVE_GSL
    gsl_matrix_complex_view gsl_C = gsl_matrix_complex_view_array_with_tda(
            (double *)d->C, d->usize, d->usize, d->umax);
    CHECK_GSL_STATUS(gsl_matrix_complex_transpose(&gsl_C.matrix));
    CHECK_GSL_STATUS(gsl_linalg_complex_cholesky_decomp(&gsl_C.matrix));
#else
#  error "no linear algebra library"
#endif

    return unew;
}