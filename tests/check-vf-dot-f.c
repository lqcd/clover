#include <stdio.h>
#include <stdlib.h>
#include <clover.h>
#include <math.h>
#include "vfermion-test.h"

static void
test_dot_f(const char *name,
           int esize, int width,
           struct vFermion *v,
           struct Fermion *f,
           struct Fermion *t,
           int b, int l)
{
    int i;
    int z_len = 2 * width;
    double *vf_n = malloc(z_len * sizeof (double));
    double n_f, n_t;
    double ff_r, ff_i;
    double delta;
    double m_e = 0;

    memset(vf_n, 0, z_len * sizeof (double));
    qx(do_fvH_dot_f)(esize, vf_n, v, width, b, l, f);
    n_f = 0;
    qx(f_norm)(&n_f, esize, f);
    for (i = 0; i < l; i++) {
        qx(fv_get)(esize, t, v, width, i + b);
        ff_r = ff_i = 0;
        qx(f_dot)(&ff_r, &ff_i, esize, t, f);
        n_t = 0;
        qx(f_norm)(&n_t, esize, t);
        ff_r -= vf_n[2 * i];
        ff_i -= vf_n[2 * i + 1];
        delta = (ff_r * ff_r + ff_i * ff_i) / sqrt(n_f * n_t);
        printf("%s %3d %15.7e\n", name, i, delta);
        if (delta > m_e)
            m_e = delta;
    }
    printf("Max error: %s %15.7e\n", name, m_e);
}

int
main(int argc, char *argv[])
{
    int esize;
    int width;
    int v_begin, v_len;
    struct vFermion *v0;
    struct vFermion *v1;
    struct Fermion *f0;
    struct Fermion *tmp;

    if (argc != 5) {
        fprintf(stderr, "Usage: %s esize width begin len\n", argv[0]);
        return 1;
    }
    esize = atoi(argv[1]);
    width = atoi(argv[2]);
    v_begin = atoi(argv[3]);
    v_len = atoi(argv[4]);

    f0 = new_fermion(esize);
    tmp = new_fermion(esize);
    v0 = new_vfermion(esize, width);
    v1 = new_vfermion(esize, 2 * width);

    construct_vf(esize, width, v0, tmp, 2.45);
    construct_vf(esize, 2 * width, v1, tmp, .74565);
    construct_f(esize, f0, 7.534121);

    test_dot_f("test A", esize, width, v0, f0, tmp, v_begin, v_len);
    test_dot_f("test B", esize, 2 * width, v1, f0, tmp, v_begin, v_len);

#if 0
    show_vfermion("V0", v0, esize, width, tmp);
    show_vfermion("V1", v1, esize, 2 * width, tmp);
#endif

    return 0;
}
