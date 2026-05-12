#include <sycl/ext/altera/fpga_extensions.hpp>
#include <sycl/sycl.hpp>

#include <algorithm>
#include <iostream>
#include <vector>

#include "exception_handler.hpp"

namespace altera_exp = sycl::ext::altera::experimental;

// Default values for the buffer size are based on a reasonable runtime for
// different targets
#if defined(FPGA_SIMULATOR)
constexpr size_t kArraySize = 1 << 8;
#elif defined(FPGA_EMULATOR)
constexpr size_t kArraySize = 1 << 12;
#else
constexpr size_t kArraySize = 1 << 20;
#endif

constexpr size_t kNumSignals = kArraySize >> 4;

// Forward declare kernel and pipe names to reduce name mangling
class ProducerKernelID;
class ConsumerKernelID;
class DataPipeID;
class SignalPipeID;

///////////////////////////////////////////////////////////////////////////////
// Pipe declarations
///////////////////////////////////////////////////////////////////////////////

// Inter-kernel pipes
// Data pipe: blocking write by producer, blocking read by consumer
using DataPipe = altera_exp::pipe<
    DataPipeID,           // Pipe identifier
    int,                  // Data type
    8>;                   // Minimum pipe capacity
// Signal pipe: blocking write by producer (every 16th element),
// non-blocking read by consumer. Represents intermittent data that may or
// may not be available on any given iteration.
using SignalPipe = altera_exp::pipe<
    SignalPipeID,         // Pipe identifier
    bool,                 // Data type
    16>;                   // Minimum pipe capacity

///////////////////////////////////////////////////////////////////////////////
// Producer kernel
//   - Blocking write of each data element into DataPipe
//   - Blocking write of a signal into SignalPipe every 16th element
///////////////////////////////////////////////////////////////////////////////
void Producer(sycl::queue &q, sycl::buffer<int, 1> &input_buffer) {
  q.submit([&](sycl::handler &h) {
    sycl::accessor input_accessor(input_buffer, h, sycl::read_only);
    size_t num_elements = input_buffer.size();

    h.single_task<ProducerKernelID>([=]() {
      for (size_t i = 0; i < num_elements; ++i) {
        // Write the signal before the data so that it is already visible in
        // SignalPipe by the time the consumer's blocking DataPipe read returns.
        if (i % (1 << 4) == 0)
          SignalPipe::write(true);

        DataPipe::write(input_accessor[i]);
      }
    });
  });
}

///////////////////////////////////////////////////////////////////////////////
// Consumer kernel
//   - Blocking read of each data element from DataPipe
//   - Non-blocking read from SignalPipe: represents intermittent data
//     that arrives depending on the relative execution timing of the two
//     kernels. The consumer counts how many signals it received.
///////////////////////////////////////////////////////////////////////////////

// An example of some simple work, to be done by the Consumer kernel on the
// input data
int ConsumerWork(int i) { return i * i; }

void Consumer(sycl::queue &q, sycl::buffer<int, 1> &output_buffer,
              sycl::buffer<int, 1> &signal_count_buffer) {
  q.submit([&](sycl::handler &h) {
    sycl::accessor out_accessor(
        output_buffer, h, sycl::write_only, sycl::no_init);
    sycl::accessor count_accessor(
        signal_count_buffer, h, sycl::write_only, sycl::no_init);
    size_t num_elements = output_buffer.size();

    h.single_task<ConsumerKernelID>([=]() {
      int signal_count = 0;

      for (size_t i = 0; i < num_elements; ++i) {
        // Blocking read: waits until producer has written data
        int data = DataPipe::read();
        out_accessor[i] = ConsumerWork(data);

        // Non-blocking read: check if signal data is available.
        // Signal data arrives intermittently, the Consumer cannot predict
        // the arrival of the signal.
        bool got_signal = false;
        SignalPipe::read(got_signal); // We don't need the actual signal value
        if (got_signal) signal_count++;
      }

      count_accessor[0] = signal_count;
    });
  });
}

///////////////////////////////////////////////////////////////////////////////
// Host testbench
///////////////////////////////////////////////////////////////////////////////
int main() {
  bool passed = true;
  int signal_count = 0;

  std::vector<int> producer_input(kArraySize);
  std::vector<int> consumer_output(kArraySize);

  constexpr int kMaxVal = 46340;
  std::generate(producer_input.begin(), producer_input.end(),
                []() { return rand() % kMaxVal; });

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

    std::cout << "Processing " << kArraySize << " elements" << std::endl;

    sycl::buffer producer_buffer(producer_input);
    sycl::buffer consumer_buffer(consumer_output);
    sycl::buffer signal_count_buffer(&signal_count, sycl::range<1>(1));

    // Run the two kernels concurrently
    Producer(q, producer_buffer);
    Consumer(q, consumer_buffer, signal_count_buffer);
  } catch (sycl::exception const &e) {
    std::cerr << "Caught a synchronous SYCL exception: " << e.what()
              << std::endl;
    std::cerr << "   If you are targeting an FPGA hardware, "
                 "ensure that your system is plugged to an FPGA board that is "
                 "set up correctly"
              << std::endl;
    std::terminate();
  }

  // Verify the signal count
  std::cout << "Signals received: " << signal_count << " / " << kNumSignals
            << std::endl;

  if (signal_count != static_cast<int>(kNumSignals)) {
    std::cout << "ERROR: expected " << kNumSignals << " signals, got "
              << signal_count << std::endl;
    passed = false;
  }

  // Verify the computation results
  for (size_t i = 0; i < kArraySize; i++) {
    if (consumer_output[i] != ConsumerWork(producer_input[i])) {
      std::cout << "idx=" << i << ": result " << consumer_output[i]
                << ", expected " << ConsumerWork(producer_input[i])
                << std::endl;
      passed = false;
    }
  }

  std::cout << (passed ? "PASSED" : "FAILED") << std::endl;

  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
