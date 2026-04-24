//==============================================================
// Copyright Altera Corporation. All rights reserved.
//
// SPDX-License-Identifier: MIT
// =============================================================

#include <iostream>
#include <vector>

#include <sycl/sycl.hpp>
#include <sycl/ext/altera/fpga_extensions.hpp>

#include "exception_handler.hpp"

using namespace sycl;

// the tolerance used in floating point comparisons
constexpr float kTol = 0.001;

// the array size of vectors a, b and c
constexpr size_t kArraySize = 32;

// Forward declare the kernel names in the global scope. This FPGA best practice
// reduces compiler name mangling in the optimization reports.
class VectorAdd;

void RunKernel(queue& q, buffer<float,1>& buf_a, buffer<float,1>& buf_b,
               buffer<float,1>& buf_r, size_t size){
    // submit the kernel
    q.submit([&](handler &h) {
      // Data accessors
      accessor a(buf_a, h, read_only);
      accessor b(buf_b, h, read_only);
      accessor r(buf_r, h, write_only, no_init);

      // Kernel executes with pipeline parallelism on the FPGA.
      // Use kernel_args_restrict to specify that a, b, and r do not alias.
      h.single_task<VectorAdd>([=]() [[intel::kernel_args_restrict]] {
        for (size_t i = 0; i < size; ++i) {
          r[i] = a[i] + b[i];
        }
      });
    });
}

int main() {
  std::vector<float> vec_a(kArraySize);
  std::vector<float> vec_b(kArraySize);
  std::vector<float> vec_r(kArraySize);

  // Fill vectors a and b with random float values
  for (size_t i = 0; i < kArraySize; i++) {
    vec_a[i] = rand() / (float)RAND_MAX;
    vec_b[i] = rand() / (float)RAND_MAX;
  }

  // Select either the FPGA emulator, FPGA simulator or FPGA device
#if FPGA_SIMULATOR
  auto selector = sycl::ext::altera::fpga_simulator_selector_v;
#elif FPGA_HARDWARE
  auto selector = sycl::ext::altera::fpga_selector_v;
#else  // #if FPGA_EMULATOR
  auto selector = sycl::ext::altera::fpga_emulator_selector_v;
#endif

  try {

    // Create a queue bound to the chosen device.
    // If the device is unavailable, a SYCL runtime exception is thrown.
    queue q(selector, fpga_tools::exception_handler);

    auto device = q.get_device();

    std::cout << "Running on device: "
              << device.get_info<sycl::info::device::name>().c_str()
              << std::endl;

    // create the device buffers
    buffer device_a(vec_a);
    buffer device_b(vec_b);
    buffer device_r(vec_r);

    RunKernel(q, device_a, device_b, device_r, kArraySize);

  } catch (exception const &e) {
    // Catches exceptions in the host code
    std::cerr << "Caught a SYCL host exception:\n" << e.what() << "\n";

    // Most likely the runtime couldn't find FPGA hardware!
    if (e.code().value() == CL_DEVICE_NOT_FOUND) {
      std::cerr << "If you are targeting an FPGA, please ensure that your "
                   "system has a correctly configured FPGA board.\n";
      std::cerr << "Run sys_check in the oneAPI root directory to verify.\n";
      std::cerr << "If you are targeting the FPGA emulator, compile with "
                   "-DFPGA_EMULATOR.\n";
    }
    std::terminate();
  }

  // At this point, the device buffers have gone out of scope and the kernel
  // has been synchronized. Therefore, the output data (vec_r) has been updated
  // with the results of the kernel and is safely accesible by the host CPU.

  // Test the results
  size_t correct = 0;
  for (size_t i = 0; i < kArraySize; i++) {
    float tmp = vec_a[i] + vec_b[i] - vec_r[i];
    if (tmp * tmp < kTol * kTol) {
      correct++;
    }
  }

  // Summarize results
  if (correct == kArraySize) {
    std::cout << "PASSED: results are correct\n";
  } else {
    std::cout << "FAILED: results are incorrect\n";
  }

  return !(correct == kArraySize);
}
