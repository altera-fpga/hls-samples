#include <iostream>

#include <sycl/ext/altera/fpga_extensions.hpp>
#include <sycl/sycl.hpp>

#include "exception_handler.hpp"

// Define namespace alias for easy reference.
namespace altera_exp = sycl::ext::altera::experimental;
namespace oneapi_exp = sycl::ext::oneapi::experimental;

constexpr int kVectorSize = 256;

// Forward declare the kernel name in the global scope. This is an FPGA best
// practice that reduces name mangling in the optimization reports.
class IDSimpleVAdd;

struct SimpleVAddKernel {
  oneapi_exp::annotated_arg<
      int *,
      decltype(oneapi_exp::properties{altera_exp::conduit})
  > a_in;

  oneapi_exp::annotated_arg<
      int *,
      decltype(oneapi_exp::properties{altera_exp::conduit})
  > b_in;

  oneapi_exp::annotated_arg<
      int *,
      decltype(oneapi_exp::properties{altera_exp::conduit})
  > c_out;

  oneapi_exp::annotated_arg<
      int,
      decltype(oneapi_exp::properties{altera_exp::conduit})
  > len;

  // kernel property method to config invocation interface
  auto get(oneapi_exp::properties_tag) {
    return oneapi_exp::properties{altera_exp::streaming_interface<>};
  }

  void operator()() const {
    for (int idx = 0; idx < len; idx++) {
      int a_val = a_in[idx];
      int b_val = b_in[idx];
      int sum = a_val + b_val;
      c_out[idx] = sum;
    }
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

    // Vector size is a constant here, but it could be a run-time variable too.
    int count = kVectorSize;

    // Create USM shared allocations in the specified buffer_location.
    // You can also use host allocations with malloc_host(...) API
    int *a = sycl::malloc_shared<int>(count, q);
    int *b = sycl::malloc_shared<int>(count, q);
    int *c = sycl::malloc_shared<int>(count, q);
    for (int i = 0; i < count; i++) {
      a[i] = i;
      b[i] = (count - i);
    }

    std::cout << "Add two vectors of size " << count << std::endl;

    sycl::event e = q.single_task<IDSimpleVAdd>(SimpleVAddKernel{a, b, c, count});

    // Verify that outputs are correct, after the kernel has finished running.
    e.wait();
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
