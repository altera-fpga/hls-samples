#include <iostream>

#include <sycl/ext/altera/fpga_extensions.hpp>
#include <sycl/sycl.hpp>

#include "exception_handler.hpp"

// Define namespace alias for easy reference.
namespace altera_exp = sycl::ext::altera::experimental;
namespace oneapi_exp = sycl::ext::oneapi::experimental;

constexpr int kVectorSize = 256;
constexpr int kBatchSize = 64;

// Declare a device-global index variable
using DeviceGlobalProperties = decltype(oneapi_exp::properties(
  oneapi_exp::device_image_scope, oneapi_exp::host_access_write));
oneapi_exp::device_global<int, DeviceGlobalProperties> start_idx;

// Forward declare the kernel name in the global scope. This is an FPGA best
// practice that reduces name mangling in the optimization reports.
class IDSimpleVAdd;

struct SimpleVAddKernel {
  int *a_in;
  int *b_in;
  int *c_out;
  int len;

  void operator()() const {
    const int start = start_idx.get();
    for (int idx = start; idx < start + len; idx++) {
      int a_val = a_in[idx];
      int b_val = b_in[idx];
      int sum = a_val + b_val;
      c_out[idx] = sum;
    }
    // Update idx, state will be preserved across kernel invocations
    start_idx = start + len;
  }
};

int main() {
  bool passed = true;
  try {
    // Use compile-time macros to select either:
    //  - the FPGA emulator device (CPU emulation of the FPGA)
    //  - the FPGA device (a real FPGA)
    //  - the simulator device
#if FPGA_SIMULATOR
    auto selector = sycl::ext::altera::fpga_simulator_selector_v;
#elif FPGA_HARDWARE
    auto selector = sycl::ext::altera::fpga_selector_v;
#else  // #if FPGA_EMULATOR
    auto selector = sycl::ext::altera::fpga_emulator_selector_v;
#endif

    // create the device queue
    sycl::queue q(selector, fpga_tools::exception_handler);

    auto device = q.get_device();

    std::cout << "Running on device: "
              << device.get_info<sycl::info::device::name>().c_str()
              << std::endl;

    // Vector size and batch size are constants here, but they could be a
    // run-time variable too.
    int count = kVectorSize;
    int batch = kBatchSize;
    int num_batches = count / batch;

    // Create USM shared allocations in the specified buffer_location.
    // You can also use host allocations with malloc_host(...) API
    int *a = sycl::malloc_shared<int>(count, q);
    int *b = sycl::malloc_shared<int>(count, q);
    int *c = sycl::malloc_shared<int>(count, q);
    for (int i = 0; i < count; i++) {
      a[i] = i;
      b[i] = (count - i);
    }

    // Initialize the device-global index variable
    int init_idx = 0;
    q.copy(&init_idx, start_idx).wait();

    std::cout << "Add two vectors of size " << count << " in " << num_batches
              << " batches of size " << batch << std::endl;

    for (int i = 0; i < num_batches; i++)
      q.single_task<IDSimpleVAdd>(SimpleVAddKernel{a, b, c, batch}).wait();

    // Verify that outputs are correct, after the last invocation of the kernel
    // has finished running.
    for (int i = 0; i < count; i++) {
      int expected = a[i] + b[i];
      if (c[i] != expected) {
        std::cout << "idx=" << i << ": result " << c[i] << ", expected ("
                  << expected << ") A=" << a[i] << " + B=" << b[i] << std::endl;
        passed = false;
      }
    }

    std::cout << (passed ? "PASSED" : "FAILED") << std::endl;

    sycl::free(a, q);
    sycl::free(b, q);
    sycl::free(c, q);

  } catch (sycl::exception const &e) {
    std::cerr << "Caught a synchronous SYCL exception: " << e.what()
              << std::endl;
    std::terminate();
  }

  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
