#include <sycl/ext/altera/fpga_extensions.hpp>
#include <sycl/sycl.hpp>

#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>

#include "exception_handler.hpp"

namespace altera_exp = sycl::ext::altera::experimental;
namespace oneapi_exp = sycl::ext::oneapi::experimental;

constexpr int kVectorSize = 256;

#if FPGA_EMULATOR
constexpr unsigned int kPipeMinCapacity = kVectorSize;
#else
constexpr unsigned int kPipeMinCapacity = 0;
#endif

// Forward declare kernel and pipe names to reduce name mangling
class PassthroughKernelID;
class InputPipeID;
class OutputPipeID;
class StopCSRID;
class CountCSRID;

///////////////////////////////////////////////////////////////////////////////
// Streaming data pipes (Avalon streaming, the default protocol)
///////////////////////////////////////////////////////////////////////////////
using InputPipe = altera_exp::pipe<InputPipeID, int, kPipeMinCapacity>;
using OutputPipe = altera_exp::pipe<OutputPipeID, int, kPipeMinCapacity>;

///////////////////////////////////////////////////////////////////////////////
// CSR control pipes (Avalon memory-mapped protocol)
///////////////////////////////////////////////////////////////////////////////

// Input CSR properties: uses_valid ensures the kernel only consumes a new value
// after the host explicitly writes one. uses_ready lets the host check whether
// the kernel has consumed the previous value.
using CsrInProperties = decltype(oneapi_exp::properties(
    altera_exp::protocol_avalon_mm,
    altera_exp::uses_valid_on));

// Output CSR properties: uses_valid lets the host verify the data is valid.
// uses_ready is disabled so the kernel can update the register without waiting
// for the host to acknowledge -- the host simply polls at its own pace.
using CsrOutProperties = decltype(oneapi_exp::properties(
    altera_exp::protocol_avalon_mm,
    altera_exp::uses_ready_off));

// Input CSR: tells the kernel to gracefully stop
using StopCSR = altera_exp::pipe<StopCSRID, bool, 0, CsrInProperties>;

// Output CSR: kernel publishes the number of elements it has processed
using CountCSR = altera_exp::pipe<CountCSRID, int, 0, CsrOutProperties>;

///////////////////////////////////////////////////////////////////////////////
// Kernel: a long running processing loop controlled by CSR pipes
///////////////////////////////////////////////////////////////////////////////
struct PassthroughKernel {
  void operator()() const {
    bool keep_going = true;
    int processed_count = 0;

    while (keep_going) {
      // All pipe reads in this loop are non-blocking so the kernel never
      // stalls waiting for data -- this ensures the stop CSR is checked on
      // every iteration and the host can interrupt the kernel at any time.
      bool got_data = false;
      int data = InputPipe::read(got_data);

      // Non-blocking read of the stop CSR
      bool got_stop = false;
      bool should_stop = StopCSR::read(got_stop);

      // Process data when available
      if (got_data) {
        OutputPipe::write(data);
        processed_count++;
        CountCSR::write(processed_count);
      }

      if (got_stop) {
        keep_going = !should_stop;
      }
    }
  }
};

///////////////////////////////////////////////////////////////////////////////
// Host testbench
///////////////////////////////////////////////////////////////////////////////
int main() {
  bool passed = true;

  int count = kVectorSize;

  std::vector<int> a(count);
  std::iota(a.begin(), a.end(), 1);

  try {
#if FPGA_SIMULATOR
    auto selector = sycl::ext::altera::fpga_simulator_selector_v;
#elif FPGA_HARDWARE
    auto selector = sycl::ext::altera::fpga_selector_v;
#else  // #if FPGA_EMULATOR
    auto selector = sycl::ext::altera::fpga_emulator_selector_v;
#endif

    sycl::queue q(selector, fpga_tools::exception_handler);

    auto device = q.get_device();
    std::cout << "Running on device: "
              << device.get_info<sycl::info::device::name>().c_str()
              << std::endl;

    std::cout << "Processing " << count << " elements" << std::endl;

    // Launch the long running kernel
    sycl::event e = q.single_task<PassthroughKernelID>(PassthroughKernel{});

    // Stream all input data to the kernel
    for (int i = 0; i < count; i++) {
      InputPipe::write(q, a[i]);
    }

    // Read all results and verify
    for (int i = 0; i < count; i++) {
      int result = OutputPipe::read(q);
      if (result != a[i]) {
        std::cout << "idx=" << i << ": result " << result
                  << ", expected " << a[i] << std::endl;
        passed = false;
      }
    }

    // Read the processed count from the output CSR
    int final_count = CountCSR::read(q);
    std::cout << "Kernel processed " << final_count << " elements" << std::endl;

    // Tell the kernel to stop
    StopCSR::write(q, true);
    e.wait();

    std::cout << (passed ? "PASSED" : "FAILED") << std::endl;

  } catch (sycl::exception const &e) {
    std::cerr << "Caught a synchronous SYCL exception: " << e.what()
              << std::endl;
    std::cerr << "   If you are targeting an FPGA hardware, "
                 "ensure that your system is plugged to an FPGA board that is "
                 "set up correctly"
              << std::endl;
    std::terminate();
  }

  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
