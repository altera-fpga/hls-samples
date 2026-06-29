#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>

#include <sycl/ext/altera/fpga_extensions.hpp>
#include <sycl/sycl.hpp>

#include "exception_handler.hpp"

// Define namespace alias for easy reference.
namespace altera_exp = sycl::ext::altera::experimental;
namespace oneapi_exp = sycl::ext::oneapi::experimental;

constexpr int kVectorSize = 256;
#if FPGA_EMULATOR
// Specify non-zero capacity for emulator to avoid unexpected hang
constexpr unsigned int kPipeMinCapacity = kVectorSize;
#else
constexpr unsigned int kPipeMinCapacity = 0;
#endif

// Forward declare the kernel name in the global scope. This is an FPGA best
// practice that reduces name mangling in the optimization reports.
class IDSimpleVAdd;

// Forward declare pipe names to reduce name mangling
class IDPipeA;
class IDPipeB;
class IDPipeC;

// Host pipes with CSR interface properties
using CsrProperties = decltype(oneapi_exp::properties(
    altera_exp::protocol<altera_exp::protocol_name::avalon_mm>));
using InputPipeA =
    altera_exp::pipe<IDPipeA, int, kPipeMinCapacity, CsrProperties>;
using InputPipeB =
    altera_exp::pipe<IDPipeB, int, kPipeMinCapacity, CsrProperties>;
using OutputPipeC =
    altera_exp::pipe<IDPipeC, int, kPipeMinCapacity, CsrProperties>;

struct SimpleVAddKernel {
  int len;

  void operator()() const {
    for (int idx = 0; idx < len; idx++) {
      int a_val = InputPipeA::read();
      int b_val = InputPipeB::read();
      int sum = a_val + b_val;
      OutputPipeC::write(sum);
    }
  }
};

int main() {
  bool passed = true;

  // Vector size is a constant here, but it could be a run-time variable too.
  int count = kVectorSize;

  // Fill vectors with values from 0 to count - 1 and count to 1
  std::vector<int> a(count);
  std::vector<int> b(count);
  std::iota(a.begin(), a.end(), 0);
  std::generate(b.begin(), b.end(),
                [i = 0, count]() mutable { return count - (i++); });

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

    std::cout << "Add two vectors of size " << count << std::endl;
    q.single_task<IDSimpleVAdd>(SimpleVAddKernel{count});

    // CSR pipes are register-based interfaces with a depth of 1, not
    // FIFO-based streaming constructs. The host and kernel must exchange
    // data one element at a time in lockstep: the host writes an element,
    // the kernel reads it, and vice versa for outputs.
    for (int i = 0; i < count; i++) {
      // When writing to a host pipe in non kernel code,
      // you must pass the sycl::queue as the first argument
      InputPipeA::write(q, a[i]);
      InputPipeB::write(q, b[i]);

      // Verify that outputs are correct.
      int expected = a[i] + b[i];
      int calc = OutputPipeC::read(q);
      if (calc != expected) {
        std::cout << "idx=" << i << ": result " << calc << ", expected ("
                  << expected << ") A=" << a[i] << " + B=" << b[i] << std::endl;
        passed = false;
      }
    }

    std::cout << (passed ? "PASSED" : "FAILED") << std::endl;
  } catch (sycl::exception const &e) {
    std::cerr << "Caught a synchronous SYCL exception: " << e.what()
              << std::endl;
    std::terminate();
  }

  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
