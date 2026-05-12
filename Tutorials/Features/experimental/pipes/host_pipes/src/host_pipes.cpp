#include <sycl/ext/altera/fpga_extensions.hpp>
#include <sycl/sycl.hpp>

#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>

#include "exception_handler.hpp"

namespace altera_exp = sycl::ext::altera::experimental;
namespace oneapi_exp = sycl::ext::oneapi::experimental;

constexpr size_t kCount = 16;
constexpr size_t kPipeMinCapacity = kCount;

// Forward declare kernel and pipe names to reduce name mangling
class LoopBackKernelID;
class InputPipeID;
class OffsetPipeID;
class OutputPipeID;

///////////////////////////////////////////////////////////////////////////////
// Host pipe declarations
///////////////////////////////////////////////////////////////////////////////

// All properties set to their default values.
using DefaultProperties = decltype(oneapi_exp::properties(
    altera_exp::protocol_avalon_streaming,
    altera_exp::bits_per_symbol<8>,
    altera_exp::first_symbol_in_high_order_bits<false>,
    altera_exp::uses_valid_on,
    altera_exp::uses_ready_on,
    altera_exp::ready_latency<0>));

// Pipe for streaming data from external system to the kernel
using InputPipe = altera_exp::pipe<
    InputPipeID,           // Pipe identifier
    int,                   // Data type
    kPipeMinCapacity,      // Minimum pipe capacity
    DefaultProperties>;    // Pipe properties

// Pipe for streaming offset data that arrives intermittently
// to the kernel
using OffsetPipe = altera_exp::pipe<
    OffsetPipeID,          // Pipe identifier
    int,                   // Data type
    kPipeMinCapacity,      // Minimum pipe capacity
    DefaultProperties>;    // Pipe properties

// Pipe for streaming data from the kernel to external system
using OutputPipe = altera_exp::pipe<
    OutputPipeID,          // Pipe identifier
    int,                   // Data type
    kPipeMinCapacity,      // Minimum pipe capacity
    DefaultProperties>;    // Pipe properties

///////////////////////////////////////////////////////////////////////////////
// Kernel: reads from InputPipe, processes, writes to OutputPipe
///////////////////////////////////////////////////////////////////////////////

int SomethingComplicated(int val, int offset) {
  return (int)(val * sycl::sqrt(float(val)) + offset);
}

sycl::event SubmitLoopBackKernel(sycl::queue &q, size_t count) {
  return q.single_task<LoopBackKernelID>([=] {
    for (size_t i = 0; i < count; i++) {
      // Read data from input pipe
      int data = InputPipe::read();
      // Read optional parameter from offset pipe
      bool got_offset = false;
      int offset = OffsetPipe::read(got_offset);
      offset = got_offset ? offset : 0;
      // Process data with optional parameter
      int r = SomethingComplicated(data, offset);
      // Write result to output pipe
      OutputPipe::write(r);
    }
  });
}

///////////////////////////////////////////////////////////////////////////////
// Host testbench
//
// Uses the preferred simulation pattern: write all input data to the host pipe
// before launching the kernel. This ensures the host can supply data every
// clock cycle once the kernel begins executing, giving the most accurate
// simulation performance estimate.
///////////////////////////////////////////////////////////////////////////////
int main() {
  bool passed = true;

  std::vector<int> in(kCount);
  std::vector<int> offset(kCount >> 2); // Simulate 4 offsets
  std::vector<int> golden(kCount);
  std::vector<int> out(kCount, 0);

  std::generate(in.begin(), in.end(), []() { return rand() % 77; });
  std::iota(offset.begin(), offset.end(), 1); // Fill with 1, 2, ..., kCount/4
  for (size_t i = 0; i < kCount; i++) {
    int offset_val = i < (kCount >> 2) ? offset[i] : 0;
    golden[i] = SomethingComplicated(in[i], offset_val);
  }

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

    std::cout << "Processing " << kCount << " elements" << std::endl;

    // Pre-fill all input data into the host pipe before launching the kernel
    for (size_t i = 0; i < kCount; i++) {
      if (i < (kCount >> 2)) {
        OffsetPipe::write(q, offset[i]);
      }
      InputPipe::write(q, in[i]);
    }

    // Launch the kernel -- it begins consuming data immediately
    auto e = SubmitLoopBackKernel(q, kCount);

    // Read all results from the output pipe
    for (size_t i = 0; i < kCount; i++) {
      out[i] = OutputPipe::read(q);
    }

    e.wait();

  } catch (sycl::exception const &e) {
    std::cerr << "Caught a synchronous SYCL exception: " << e.what()
              << std::endl;
    std::cerr << "   If you are targeting an FPGA hardware, "
                 "ensure that your system is plugged to an FPGA board that is "
                 "set up correctly"
              << std::endl;
    std::terminate();
  }

  for (size_t i = 0; i < kCount; i++) {
    if (out[i] != golden[i]) {
      std::cout << "idx=" << i << ": result " << out[i]
                << ", expected " << golden[i] << std::endl;
      passed = false;
    }
  }

  std::cout << (passed ? "PASSED" : "FAILED") << std::endl;

  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
