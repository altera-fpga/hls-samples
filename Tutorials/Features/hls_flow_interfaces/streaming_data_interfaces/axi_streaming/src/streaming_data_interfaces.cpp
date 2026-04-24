#include <sycl/ext/altera/experimental/pipes_ext.hpp>
#include <sycl/ext/altera/fpga_extensions.hpp>
#include <sycl/sycl.hpp>

#include <iostream>

#include "exception_handler.hpp"

// Define namespace alias for easy reference.
namespace altera_exp = sycl::ext::altera::experimental;
namespace oneapi_exp = sycl::ext::oneapi::experimental;

// limit pixel values to this value, or less
constexpr int kThreshold = 200;
// Test image dimensions
constexpr unsigned int kWidth = 16;
constexpr unsigned int kHeight = 16;
#if FPGA_EMULATOR
// Specify non-zero capacity for emulator to avoid unexpected hang
constexpr unsigned int kPipeMinCapacity = kWidth * kHeight;
#else
constexpr unsigned int kPipeMinCapacity = 0;
#endif

// Forward declare the kernel and pipe names
// (this prevents unwanted name mangling in the optimization report)
class InStream;
class OutStream;
class Threshold;

// StreamingBeatAxi struct enables sideband signals in AXI streaming interface.
// StreamingBeatAxi is templated on the data type and has default values for
// the other template parameters. By default, StreamingBeatAxi uses tlast but
// not tuser signal. tuser signal can be used by setting the second template
// parameter to a custom type and setting the fourth template parameter to true:
// e.g., 
//   using StreamingBeatT =
//     altera_exp::StreamingBeatAxi<PipeDataT, PipeUserT, true, true>;
// The following declaration results in a pipe with tlast but no tuser signals.
using StreamingBeatT = altera_exp::StreamingBeatAxi<unsigned char>;

// Pipe properties
using AxiPipePropertiesT = decltype(oneapi_exp::properties(
    altera_exp::ready_latency<0>,
    altera_exp::bits_per_symbol<8>,
    altera_exp::uses_valid<true>,
    altera_exp::uses_ready<true>,
    altera_exp::first_symbol_in_high_order_bits<false>,
    altera_exp::protocol_axi_streaming));

// Image streams
using InPixelPipe = altera_exp::pipe<
    InStream,                 // An identifier for the pipe
    StreamingBeatT,           // The type of data in the pipe
    kPipeMinCapacity,         // The minimum capacity of the pipe
    AxiPipePropertiesT        // Pipe properties with AXI streaming protocol
    >;
using OutPixelPipe = altera_exp::pipe<
    OutStream,                // An identifier for the pipe
    StreamingBeatT,           // The type of data in the pipe
    kPipeMinCapacity,         // The minimum capacity of the pipe
    AxiPipePropertiesT        // Pipe properties with AXI streaming protocol
    >;

// A kernel that thresholds pixel values in an image over a stream. Uses tlast
// signal on the streams to determine the end of the image.
struct ThresholdKernel {
  void operator()() const {
    bool end_of_packet = false;

    while (!end_of_packet) {
      // Read in next pixel
      StreamingBeatT in_beat = InPixelPipe::read();
      auto pixel = in_beat.tdata;
      end_of_packet = in_beat.tlast;

      // Threshold
      if (pixel > kThreshold) pixel = kThreshold;

      // Write out result
      StreamingBeatT out_beat(pixel, end_of_packet);
      OutPixelPipe::write(out_beat);
    }
  }
};

int main() {
  bool passed = true;

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
    std::cout << std::endl;

    // Generate pixel data
    std::cout << "Writing " << (kWidth * kHeight) 
              << " pixels to the AXI input stream" << std::endl;
    for (int i = 0; i < (kWidth * kHeight); ++i) {
      bool end_of_packet = (i == ((kWidth * kHeight) - 1));
      StreamingBeatT in_beat(i, end_of_packet);
      InPixelPipe::write(q, in_beat);
    }

    // Launch the kernel
    std::cout << "Launching the kernel" << std::endl;
    q.single_task<Threshold>(ThresholdKernel{});

    // Check that output pixels are below the threshold
    std::cout << "Checking that output pixels are below the threshold"
              << std::endl;
    for (int i = 0; i < (kWidth * kHeight); ++i) {
      StreamingBeatT out_beat = OutPixelPipe::read(q);
      passed &= (out_beat.tdata <= kThreshold);
    }
    std::cout << std::endl;
  } catch (sycl::exception const &e) {
    // Catches exceptions in the host code
    std::cerr << "Caught a SYCL host exception:\n" << e.what() << "\n";
    std::terminate();
  }

  std::cout << (passed ? "PASSED" : "FAILED") << std::endl;
  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
