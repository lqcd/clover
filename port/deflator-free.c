#include <clover.h>

void
q(df_free)(struct Q(Deflator) **deflator_ptr)
{
    struct Q(State) *s;

    if (deflator_ptr == 0 || *deflator_ptr == 0)
        return;
    s = (*deflator_ptr)->state;
    BEGIN_TIMING(s);

    struct Q(Deflator) *d = *deflator_ptr;
    
    /* XXX free other components of the deflator */
#define guarded_free(v, cmd) { if (NULL == v) cmd; }
    int vmax    = d->vmax;
    int umax    = d->umax;
#define ds      sizeof(double)
#define zs      sizeof(doublecomplex)
    if (!latmat_c_is_null(&(d->V))) latmat_c_free(s, &(d->V));
    if (NULL != d->T)               q(free)(s, d->T, vmax * vmax * zs);

    if (!latmat_c_is_null(&(d->U))) latmat_c_free(s, &(d->U));
    if (NULL != d->H)               q(free)(s, d->H, umax * umax * zs);
    if (NULL != d->C)               q(free)(s, d->C, umax * umax * zs);

    if (NULL != d->hevecs2)         q(free)(s, d->hevecs2, vmax * vmax * zs);
    if (NULL != d->hevals)          q(free)(s, d->hevals, vmax * ds);
    if (NULL != d->zwork)           q(free)(s, d->zwork, d->lwork * zs);

#if defined(HAVE_LAPACK)
    if (NULL != d->hevecs1)         q(free)(s, d->hevecs1, vmax * vmax * zs);
    if (NULL != d->tau)             q(free)(s, d->tau, vmax * zs);
    if (NULL != d->rwork)           q(free)(s, d->rwork, 3 * vmax * ds);
#elif defined(HAVE_GSL)
    if (NULL != d->zwork2)          q(free)(s, d->zwork2, umax * zs);
    if (NULL != d->gsl_T_full)      gsl_matrix_complex_free(d->gsl_T_full);
    if (NULL != d->gsl_hevecs1)     gsl_matrix_complex_free(d->gsl_hevecs1);
    if (NULL != d->gsl_hevals1)     gsl_vector_free(d->gsl_hevals1);
    if (NULL != d->gsl_wkspace1)    gsl_eigen_hermv_free(d->gsl_wkspace1);
    if (NULL != d->gsl_T_m1)        gsl_matrix_complex_free(d->gsl_T_m1);
    if (NULL != d->gsl_hevecs2)     gsl_matrix_complex_free(d->gsl_hevecs2);
    if (NULL != d->gsl_hevals2)     gsl_vector_free(d->gsl_hevals2);
    if (NULL != d->gsl_wkspace2)    gsl_eigen_hermv_free(d->gsl_wkspace2);
    if (NULL != d->gsl_T_proj)      gsl_matrix_complex_free(d->gsl_T_proj);
    if (NULL != d->gsl_hevecs3)     gsl_matrix_complex_free(d->gsl_hevecs3);
    if (NULL != d->gsl_wkspace3)    gsl_eigen_hermv_free(d->gsl_wkspace3);
    if (NULL != d->gsl_QR)          gsl_matrix_complex_free(d->gsl_QR);
    if (NULL != d->gsl_Q_unpack)    gsl_matrix_complex_free(d->gsl_Q_unpack);
    if (NULL != d->gsl_tmp_MxS)     gsl_matrix_complex_free(d->gsl_tmp_MxS);
    if (NULL != d->gsl_tau)         gsl_vector_complex_free(d->gsl_tau);
    if (NULL != d->hevals_select1)  q(free)(s, d->hevals_select1, vmax * sizeof(d->hevals_select1[0]));
    if (NULL != d->hevals_select2)  q(free)(s, d->hevals_select2, vmax * sizeof(d->hevals_select2[0]));
#else
#  error "no linear algebra library"
#endif

    if (!latmat_c_is_null(&(d->tmp_V)))     latmat_c_free(s, &(d->tmp_V));
    if (!latvec_z_is_null(&(d->work_z_1)))  latvec_z_free(s, &(d->work_z_1));
    if (!latvec_z_is_null(&(d->work_z_2)))  latvec_z_free(s, &(d->work_z_2));
    if (!latvec_z_is_null(&(d->work_z_3)))  latvec_z_free(s, &(d->work_z_3));
    if (!latvec_c_is_null(&(d->work_c_1)))  latvec_c_free(s, &(d->work_c_1));
    if (!latvec_c_is_null(&(d->work_c_2)))  latvec_c_free(s, &(d->work_c_2));
    if (!latvec_c_is_null(&(d->work_c_3)))  latvec_c_free(s, &(d->work_c_3));


    END_TIMING(s, 0, 0, 0);
    *deflator_ptr = 0;
}
