#ifndef __MEMORY_TRANSFERS_HPP__
#define __MEMORY_TRANSFERS_HPP__

// Included from include/
#include "constexpr_math.hpp"
#include "tuple.hpp"
#include "unrolled_loop.hpp"

/*
  Read matrix_count matrices of type TT from DDR by bursts of num_elem_per_bank
  elements, and write the matrices to the "MatrixPipe" pipe num_elem_per_bank by
  num_elem_per_bank elements.
  Repeat these operations "repetitions" times.
*/
template <typename TT,            // Datatype of the elements of the matrix
          int rows,               // Number of rows of the matrix
          int columns,            // Number of columns of the matrix
          int num_elem_per_bank,  // Number of TT elements per DDR burst access
          typename MatrixPipe     // Output matrix pipe
          >
void MatrixReadFromDDRToPipe(
    TT* matrix_ptr,    // Input matrix pointer
    int matrix_count,  // Number of matrices to read from DDR
    int repetitions    // Number of times to write the same matrix to the pipe
) {
  // We may perform an incomplete memory read if the number of elements per row
  // is not a multiple of the DDR burst size
  constexpr bool kIncompleteBurst = (rows % num_elem_per_bank) != 0;
  constexpr int kExtraIteration = kIncompleteBurst ? 1 : 0;
  // Number of DDR burst reads of num_elem_per_bank elements required to read a
  // full column
  constexpr int kLoopIterPerColumn =
      (rows / num_elem_per_bank) + kExtraIteration;
  // Number of DDR burst reads of num_elem_per_bank to read all the matrices
  constexpr int kLoopIter = kLoopIterPerColumn * columns;
  // Size in bits of the loop iterator over kLoopIter iterations
  constexpr int kLoopIterBitSize = fpga_tools::BitsForMaxValue<kLoopIter + 1>();
  // Size of a full matrix
  constexpr int kMatrixSize = rows * columns;

#if defined (IS_BSP)
          // When targeting a BSP, we instruct the compiler that this pointer
          // lives on the device.
          // Knowing this, the compiler won't generate hardware to
          // potentially get data from the host.
          sycl::ext::intel::device_ptr<TT> matrix_ptr_located(matrix_ptr);
#else
          // Device pointers are not supported when targeting an FPGA 
          // family/part
          TT* matrix_ptr_located(matrix_ptr);
#endif  

  // Repeatedly read matrix_count matrices from the DDR and send them to the
  // pipe
  for (int repetition = 0; repetition < repetitions; repetition++) {
    for (int matrix_index = 0; matrix_index < matrix_count; matrix_index++) {
      // Keep track of the current element index in the matrix
      // Only useful in the case of kIncompleteBurst
      int load_index = 0;

      [[intel::initiation_interval(1)]]  // NO-FORMAT: Attribute
      for (ac_int<kLoopIterBitSize, false> li = 0; li < kLoopIter; li++) {
        bool last_burst_of_col;
        if constexpr (kIncompleteBurst) {
          // Check if we are reading the last DDR burst of the current column
          last_burst_of_col =
              (li % kLoopIterPerColumn) == kLoopIterPerColumn - 1;
        }

        fpga_tools::NTuple<TT, num_elem_per_bank> ddr_read;

        // Perform the DDR burst read of num_elem_per_bank elements
        fpga_tools::UnrolledLoop<num_elem_per_bank>([&](auto k) {
          if constexpr (kIncompleteBurst) {
            // Check if the current read index is beyond the end of the current
            // matrix column
            bool out_of_bounds =
                last_burst_of_col &&
                ((k % num_elem_per_bank) > ((rows - 1) % num_elem_per_bank));

            // Only perform the DDR reads that are relevant (and don't access a
            // memory address that may be beyond the matrix last address)
            if (!out_of_bounds) {
              ddr_read.template get<k>() =
                  matrix_ptr_located[matrix_index * kMatrixSize + load_index +
                                    k];
            }
          } else {
            ddr_read.template get<k>() =
                matrix_ptr_located[matrix_index * kMatrixSize +
                                  (int)(li)*num_elem_per_bank + k];
          }
        });

        if constexpr (kIncompleteBurst) {
          // Update the current element index in the input matrix according
          // to the read size of the current iteration
          load_index +=
              last_burst_of_col ? rows % num_elem_per_bank : num_elem_per_bank;
        }

        MatrixPipe::write(ddr_read);
      }  // end of li

    }  // end of matrix_index
  }    // end of repetition
}

/*
  Read vector_count vectors of type TT from a pipe, num_elem_per_bank by
  num_elem_per_bank and write them to DDR by bursts of num_elem_per_bank
  elements.
  Repeat this operations "repetitions" times.
*/
template <typename TT,            // Datatype of the elements of the vector
          int vector_size,        // Number of elements in the vector
          int num_elem_per_bank,  // Number of TT elements per DDR burst access
          typename VectorPipe     // Input vector
          >
void VectorReadFromPipeToDDR(
    TT* vector_ptr,    // Output vector pointer
    int vector_count,  // Number of vectors to write to the DDR
    int repetitions    // Number of times to read the same vector from the pipe
) {
  // The number of elements in the vector may not be a multiple of
  // num_elem_per_bank so we may have to do an extra incomplete write to DDR.
  constexpr bool kIncompleteBurst = vector_size % num_elem_per_bank != 0;
  constexpr int kExtraIteration = kIncompleteBurst ? 1 : 0;
  constexpr int kLoopIter = (vector_size / num_elem_per_bank) + kExtraIteration;

#if defined (IS_BSP)
          // When targeting a BSP, we instruct the compiler that this pointer
          // lives on the device.
          // Knowing this, the compiler won't generate hardware to
          // potentially get data from the host.
          sycl::ext::intel::device_ptr<TT> vector_ptr_located(vector_ptr);
#else
          // Device pointers are not supported when targeting an FPGA 
          // family/part
          TT* vector_ptr_located(vector_ptr);
#endif  

  // Repeat vector_count complete I vector pipe reads
  // for as many repetitions as needed
  for (int rep_idx = 0; rep_idx < repetitions; rep_idx++) {
    for (int vector_idx = 0; vector_idx < vector_count; vector_idx++) {
      for (int li = 0; li < kLoopIter; li++) {
        TT bank[num_elem_per_bank];

        for (int k = 0; k < num_elem_per_bank; k++) {
          if (((li * num_elem_per_bank) + k) < vector_size) {
            bank[k] = VectorPipe::read();
          }
        }

        // Copy the I vector result to DDR
        if constexpr (kIncompleteBurst) {
// Write a burst of num_elem_per_bank elements to DDR
#pragma unroll
          for (int k = 0; k < num_elem_per_bank; k++) {
            if (((li * num_elem_per_bank) + k) < vector_size) {
              vector_ptr_located[(vector_idx * vector_size) +
                                (li * num_elem_per_bank) + k] = bank[k];
            }
          }
        } else {
// Write a burst of num_elem_per_bank elements to DDR
#pragma unroll
          for (int k = 0; k < num_elem_per_bank; k++) {
            vector_ptr_located[(vector_idx * vector_size) +
                              (li * num_elem_per_bank) + k] = bank[k];
          }
        }
      }  // end of li
    }    // end of vector_idx
  }      // end of rep_idx
}

#endif /* __MEMORY_TRANSFERS_HPP__ */
