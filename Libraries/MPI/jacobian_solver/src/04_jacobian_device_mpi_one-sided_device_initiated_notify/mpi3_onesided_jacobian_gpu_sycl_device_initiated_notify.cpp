/*==============================================================
 * Copyright © 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 * ============================================================= */

/* Distributed Jacobian computation sample using OpenMP GPU offload and MPI-3 one-sided.
 */
#include "mpi.h"

#ifndef MPI_ERR_INVALID_NOTIFICATION
/*For Intel MPI 2021.13/14 we have to use API compatibility layer*/
#include "mpix_compat.h"
#endif
#include <sycl.hpp>
#include <vector>
#include <iostream>

const int Nx = 16384; /* Grid size */
const int Ny = Nx;
const int Niter = 100; /* Nuber of algorithm iterations */
const int NormIteration = 0; /* Recaluculate norm after given number of iterations. 0 to disable norm calculation */
const int PrintTime = 1; /* Output overall time of compute/communication part */

struct subarray {
    int rank, comm_size;        /* MPI rank and communicator size */
    int x_size, y_size;         /* Subarray size excluding border rows and columns */
    MPI_Aint l_nbh_offt;        /* Offset predecessor data to update */
};

#define ROW_SIZE(S) ((S).x_size + 2)
#define XY_2_IDX(X,Y,S) (((Y)+1)*ROW_SIZE(S)+((X)+1))

/* Subroutine to create and initialize initial state of input subarrays */
void InitDeviceArrays(double **A_dev_1, double **A_dev_2, sycl::queue q, struct subarray *sub)
{
    size_t total_size = (sub->x_size + 2) * (sub->y_size + 2);

    double *A = sycl::malloc_host < double >(total_size, q);
    *A_dev_1 = sycl::malloc_device < double >(total_size, q);
    *A_dev_2 = sycl::malloc_device < double >(total_size, q);

    for (int i = 0; i < (sub->y_size + 2); i++)
        for (int j = 0; j < (sub->x_size + 2); j++)
            A[i * (sub->x_size + 2) + j] = 0.0;

    if (sub->rank == 0) /* set top boundary */
        for (int i = 1; i <= sub->x_size; i++)
            A[i] = 1.0; /* set bottom boundary */
    if (sub->rank == (sub->comm_size - 1))
        for (int i = 1; i <= sub->x_size; i++)
            A[(sub->x_size + 2) * (sub->y_size + 1) + i] = 10.0;

    for (int i = 1; i <= sub->y_size; i++) {
        int row_offt = i * (sub->x_size + 2);
        A[row_offt] = 1.0;      /* set left boundary */
        A[row_offt + sub->x_size + 1] = 1.0;    /* set right boundary */
    }

    /* Move input arrays to device */
    q.memcpy(*A_dev_1, A, sizeof(double) * total_size);
    q.memcpy(*A_dev_2, A, sizeof(double) * total_size);
    q.wait();
    sycl::free(A, q);
    A = NULL;
}

/* Setup subarray size and layout processed by current rank */
void GetMySubarray(struct subarray *sub)
{
    MPI_Comm_size(MPI_COMM_WORLD, &sub->comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &sub->rank);
    sub->y_size = Ny / sub->comm_size;
    sub->x_size = Nx;
    sub->l_nbh_offt = (sub->x_size + 2) * (sub->y_size + 1) + 1;


    int tail = sub->y_size % sub->comm_size;
    if (tail != 0) {
        if (sub->rank < tail)
            sub->y_size++;
        if ((sub->rank > 0) && ((sub->rank - 1) < tail))
            sub->l_nbh_offt += (sub->x_size + 2);
    }
}

int main(int argc, char *argv[])
{
    double t_start;
    struct subarray my_subarray = { };
    double *A_device[2] = { };
    MPI_Win win[2] = { MPI_WIN_NULL, MPI_WIN_NULL };
    int batch_iters = 0;
    int passed_iters = 0;
    double norm = 0.0;
    int provided;

    /* Initialization of runtime and initial state of data */
    sycl::queue q(sycl::gpu_selector_v);
    /* MPI_THREAD_MULTIPLE is required for device-initiated communications */
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    GetMySubarray(&my_subarray);
    InitDeviceArrays(&A_device[0], &A_device[1], q, &my_subarray);

#ifdef GROUP_SIZE_DEFAULT
    int work_group_size = GROUP_SIZE_DEFAULT;
#else
    int work_group_size =
      q.get_device().get_info<sycl::info::device::max_work_group_size>();
#endif

    if ((Nx % work_group_size) != 0) {
        if (my_subarray.rank == 0) {
            printf("For simplification, sycl::info::device::max_work_group_size should be divider of X dimention of array\n");
            printf("Please adjust matrix size, or define GROUP_SIZE_DEFAULT\n");
            printf("sycl::info::device::max_work_group_size=%d Nx=%d (%d)\n", work_group_size, Nx, work_group_size % Nx);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
    }
    /* Create RMA window using device memory */
    MPI_Win_create(A_device[0],
                   sizeof(double) * (my_subarray.x_size + 2) * (my_subarray.y_size + 2),
                   sizeof(double), MPI_INFO_NULL, MPI_COMM_WORLD, &win[0]);
    MPI_Win_create(A_device[1],
                   sizeof(double) * (my_subarray.x_size + 2) * (my_subarray.y_size + 2),
                   sizeof(double), MPI_INFO_NULL, MPI_COMM_WORLD, &win[1]);
    MPI_Win_notify_attach(&win[0], 1, MPI_INFO_NULL);
    MPI_Win_notify_attach(&win[1], 1, MPI_INFO_NULL);
    /* Start RMA exposure epoch */
    MPI_Win_lock_all(0, win[0]);
    MPI_Win_lock_all(0, win[1]);

    if (PrintTime) {
        t_start = MPI_Wtime();
    }


    int iterations_batch = (NormIteration <= 0) ? Niter : NormIteration;
    for (passed_iters = 0; passed_iters < Niter; passed_iters += iterations_batch) {

        /* Submit compute kernel to calculate next "iterations_batch" steps */
        q.submit([&](auto & h) {
            h.parallel_for(sycl::nd_range<1>(work_group_size, work_group_size),
                            [=](sycl::nd_item<1> item) {
                int local_id = item.get_local_id();
                int col_per_wg = my_subarray.x_size / work_group_size;

                int my_x_lb = col_per_wg * local_id;
                int my_x_ub = my_x_lb + col_per_wg;

                for (int k = 0; k < iterations_batch; ++k)
                {
                    int i = passed_iters + k;
                    MPI_Win cwin = win[(i + 1) % 2];
                    MPI_Count c_expected = 0;
                    double *a = A_device[i % 2];
                    double *a_out = A_device[(i + 1) % 2];

                    /* Calculate values on borders to initiate communications early */
                    for (int column = my_x_lb; column < my_x_ub;  column ++) {
                        int idx = XY_2_IDX(column, 0, my_subarray);
                        a_out[idx] = 0.25 * (a[idx - 1] + a[idx + 1]
                                             + a[idx - ROW_SIZE(my_subarray)]
                                             + a[idx + ROW_SIZE(my_subarray)]);
                        idx = XY_2_IDX(column, my_subarray.y_size - 1, my_subarray);
                        a_out[idx] = 0.25 * (a[idx - 1] + a[idx + 1]
                                             + a[idx - ROW_SIZE(my_subarray)]
                                             + a[idx + ROW_SIZE(my_subarray)]);
                    }

                    /* Perform 1D halo-exchange with neighbours */
                    if (my_subarray.rank != 0) {
                        int idx = XY_2_IDX(my_x_lb, 0, my_subarray);
                        MPI_Put_notify(&a_out[idx], col_per_wg, MPI_DOUBLE,
                            my_subarray.rank - 1, my_subarray.l_nbh_offt+my_x_lb,
                            col_per_wg, MPI_DOUBLE, 0, cwin);
                        c_expected+=work_group_size;
                     }

                     if (my_subarray.rank != (my_subarray.comm_size - 1)) {
                         int idx = XY_2_IDX(my_x_lb, my_subarray.y_size - 1, my_subarray);
                         MPI_Put_notify(&a_out[idx], col_per_wg, MPI_DOUBLE,
                              my_subarray.rank + 1, 1+my_x_lb,
                              col_per_wg, MPI_DOUBLE, 0, cwin);
                        c_expected+=work_group_size;
                     }

                    /* Recalculate internal points in parallel with comunications */
                    for (int row = 1; row < my_subarray.y_size - 1; ++row) {
                         for (int column = my_x_lb; column < my_x_ub;  column ++) {
                              int idx = XY_2_IDX(column, row, my_subarray);
                              a_out[idx] = 0.25 * (a[idx - 1] + a[idx + 1]
                                                             + a[idx - ROW_SIZE(my_subarray)]
                                                             + a[idx + ROW_SIZE(my_subarray)]);
                         }
                    }

                    item.barrier(sycl::access::fence_space::global_space);
                    if (local_id == 0){
                        MPI_Count c;
                        while (c < c_expected) MPI_Win_notify_get_value(cwin, 0, &c);
                        MPI_Win_notify_set_value(cwin, 0, 0);
                    }
                    item.barrier(sycl::access::fence_space::global_space);
                }
            });
        }).wait();

        /* Calculate and report norm value after given number of iterations */
        if ((NormIteration > 0) && ((NormIteration - 1) == i % NormIteration)) {
            double rank_norm = 0.0;

            {
                sycl::buffer<double> norm_buf(&rank_norm, 1);
                q.submit([&](auto & h) {
                    auto sumr = sycl::reduction(norm_buf, h, sycl::plus<>());
                    h.parallel_for(sycl::range(my_subarray.x_size, my_subarray.y_size), sumr, [=] (auto index, auto &v) {
                        int idx = XY_2_IDX(index[0], index[1], my_subarray);
                        double diff = a_out[idx] - a[idx];
                        v += (diff * diff);
                    });
                }).wait();
            }

            /* Get global norm value */
            MPI_Reduce(&rank_norm, &norm, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
            if (my_subarray.rank == 0) {
                printf("NORM value on iteration %d: %f\n", i+1, sqrt(norm));
            }
        }
    }

    if (PrintTime) {
        double avg_time;
        double rank_time;
        rank_time = MPI_Wtime() - t_start;

        MPI_Reduce(&rank_time, &avg_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

        if (my_subarray.rank == 0) {
            avg_time = avg_time/my_subarray.comm_size;
            printf("Average solver time: %f(sec)\n", avg_time);
        }
    }

    if (my_subarray.rank == 0) {
        printf("[%d] SUCCESS\n", my_subarray.rank);
    }

    MPI_Win_unlock_all(&win[1]);
    MPI_Win_unlock_all(&win[0]);

    MPI_Win_free(&win[1]);
    MPI_Win_free(&win[0]);
    MPI_Finalize();

    sycl::free(A_device[0], q);
    sycl::free(A_device[1], q);

    return 0;
}
