#include <clover.h>

void
qx(op_AxBx)(struct Fermion *r_x,
            struct eo_lattice *xy,
            const struct SUn *U,
            const struct Clover *C,
            const struct Fermion *a_y,
            long long *flops,
            long long *sent,
            long long *received)
{
    qx(boundary)(xy, qx(up_project_n), qx(down_project_n), U, a_y, flops);

    if (xy->h_valid)
        QMP_start(xy->handle);

    *flops += qx(do_AxBx)(r_x, 0, xy->body_size,
                          xy->neighbor, U, C, a_y, NULL);

    if (xy->h_valid)
        QMP_wait(xy->handle);

    *flops += qx(do_AxBx)(r_x, xy->body_size, xy->face_size,
                          xy->neighbor, U, C, a_y, xy->receive_buf);
    *sent += xy->total_send;
    *received += xy->total_receive;
}
