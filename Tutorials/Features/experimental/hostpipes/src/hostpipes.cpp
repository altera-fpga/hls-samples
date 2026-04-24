#include <sycl/ext/altera/fpga_extensions.hpp>
#include <sycl/sycl.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "exception_handler.hpp"

// Namespace alias
namespace oneapi_exp = sycl::ext::oneapi::experimental;
namespace altera_exp = sycl::ext::altera::experimental;

// Forward declare kernel and pipe names to reduce name mangling
class LoopBackKernelID;
class H2DPipeID;
class D2HPipeID;

// Declare host pipes
using ValueT = int;
constexpr size_t kPipeMinCapacity = 8;
constexpr size_t kBitsPerSymbol = 0; // Use a single symbol
using PipeProperties = decltype(oneapi_exp::properties(
  altera_exp::bits_per_symbol<kBitsPerSymbol>
));
using H2DPipe = altera_exp::pipe<
    H2DPipeID,         // An identifier for the pipe
    ValueT,            // The type of data in the pipe
    kPipeMinCapacity,  // The capacity of the pipe
    PipeProperties     // User-specified pipe properties
>;

using D2HPipe = altera_exp::pipe<
    D2HPipeID,         // An identifier for the pipe
    ValueT,            // The type of data in the pipe
    kPipeMinCapacity,  // The capacity of the pipe
    PipeProperties     // User-specified pipe properties
>;

// Forward declare the test functions
void AlternatingTest(sycl::queue&, ValueT*, ValueT*, size_t);
void LaunchCollectTest(sycl::queue&, ValueT*, ValueT*, size_t);

// Offloaded computation
ValueT SomethingComplicated(ValueT val) {
  return (ValueT)(val * sycl::sqrt(float(val)));
}

/////////////////////////////////////////

int main(int argc, char* argv[]) {
#if FPGA_SIMULATOR
  auto selector = sycl::ext::altera::fpga_simulator_selector_v;
#elif FPGA_HARDWARE
  auto selector = sycl::ext::altera::fpga_selector_v;
#else  // #if FPGA_EMULATOR
  auto selector = sycl::ext::altera::fpga_emulator_selector_v;
#endif

  bool passed = true;

  // Parse command line arguments
  size_t count = 16;
  if (argc > 1) {
    try {
      std::size_t idx = 0;
      const int parsed = std::stoi(argv[1], &idx);
      if (parsed <= 0 || idx != std::strlen(argv[1])) {
        std::cerr << "ERROR: 'count' must be a positive integer" << std::endl;
        return 1;
      }
      count = static_cast<size_t>(parsed);
    } catch (const std::invalid_argument &) {
      std::cerr << "ERROR: 'count' must be a positive integer" << std::endl;
      return 1;
    } catch (const std::out_of_range &) {
      std::cerr << "ERROR: 'count' is out of range" << std::endl;
      return 1;
    }
  }

  // Initialize input, golden, and output data
  std::vector<ValueT> in(count), golden(count), out_alt(count),
      out_launch(count);
  std::mt19937 gen{std::random_device{}()};
  std::uniform_int_distribution<ValueT> dist{0, 76};
  std::generate(in.begin(), in.end(), [&] { return dist(gen); });
  std::fill(out_alt.begin(), out_alt.end(), 0);
  std::fill(out_launch.begin(), out_launch.end(), 0);
  for (int i = 0; i < count; i++) {
    golden[i] = SomethingComplicated(in[i]);
  }

  // validation lambda
  auto validate = [](auto& in, auto& out, size_t size) {
    for (int i = 0; i < size; i++) {
      if (out[i] != in[i]) {
        std::cout << "\t out[" << i << "] != in[" << i << "]"
                  << " (" << out[i] << " != " << in[i] << ")" << std::endl;
        return false;
      }
    }
    return true;
  };

  try {
    // create the device queue
    sycl::queue q(selector, fpga_tools::exception_handler,
                  sycl::property::queue::enable_profiling{});

    auto device = q.get_device();
    std::cout << "Running on device: "
              << device.get_info<sycl::info::device::name>().c_str()
              << std::endl << std::endl;

    // Alternating write-and-read
    std::cout << "Running Alternating write-and-read" << std::endl;
    AlternatingTest(q, in.data(), out_alt.data(), count);

    // Launch and Collect
    std::cout << "Running Launch and Collect" << std::endl;
    LaunchCollectTest(q, in.data(), out_launch.data(),
                      std::min(count, kPipeMinCapacity));

    std::cout << std::endl;
  } catch (sycl::exception const& e) {
    // Catches exceptions in the host code
    std::cerr << "Caught a SYCL host exception:\n" << e.what() << "\n";
    std::terminate();
  }

  // Validate results
  std::cout << "Validating alternating test results" << std::endl;
  passed &= validate(golden, out_alt, count);
  std::cout << "Validating launch and collect test results" << std::endl;
  passed &= validate(golden, out_launch, std::min(count, kPipeMinCapacity));
  std::cout << std::endl;

  if (!passed) {
    std::cout << "FAILED\n";
    return 1;
  }

  std::cout << "PASSED\n";
  return 0;
}

// This kernel reads a data element from InHostPipe, processes it,
// and writes the result to OutHostPipe
template <typename KernelId,    // type identifier for kernel
          typename InHostPipe,  // host-to-device pipe
          typename OutHostPipe  // device-to-host pipe
          >
sycl::event SubmitLoopBackKernel(sycl::queue& q, size_t count) {
  return q.single_task<KernelId>([=] {
    for (size_t i = 0; i < count; i++) {
      auto d = InHostPipe::read();
      auto r = SomethingComplicated(d);
      OutHostPipe::write(r);
    }
  });
}

// This test launches SubmitLoopBackKernel, then alternates writes
// and reads to and from the H2DPipe and D2HPipe hostpipes respectively
void AlternatingTest(sycl::queue& q, ValueT* in, ValueT* out, size_t count) {
  std::cout << "\t Run Loopback Kernel on FPGA" << std::endl;
  auto e = SubmitLoopBackKernel<LoopBackKernelID, H2DPipe, D2HPipe>(q, count);

  std::cout << "\t Doing " << count << " writes & reads" << std::endl;
  for (size_t i = 0; i < count; i++) {
    // write data in host-to-device hostpipe
    H2DPipe::write(q, in[i]);
    // read data from device-to-host hostpipe
    out[i] = D2HPipe::read(q);
  }

  // No need to wait on kernel to finish as the pipe reads are blocking
  std::cout << "\t Done" << std::endl;
}

// This test launches SubmitLoopBackKernel, writes 'count' elements to H2DPipe,
// and then reads 'count' elements from D2HPipe. 'count' here must be less than
// or equal to the capacity of the pipes to make sure the writes won't hang.
void LaunchCollectTest(sycl::queue& q, ValueT* in, ValueT* out, size_t count) {
  std::cout << "\t Run Loopback Kernel on FPGA" << std::endl;

  std::cout << "\t Doing " << count << " writes" << std::endl;
  for (size_t i = 0; i < count; i++) {
    // write data in host-to-device hostpipe
    H2DPipe::write(q, in[i]);
  }

  auto e = SubmitLoopBackKernel<LoopBackKernelID, H2DPipe, D2HPipe>(q, count);

  std::cout << "\t Doing " << count << " reads" << std::endl;
  for (size_t i = 0; i < count; i++) {
    // read data from device-to-host hostpipe
    out[i] = D2HPipe::read(q);
  }

  // No need to wait on kernel to finish as the pipe reads are blocking
  std::cout << "\t Done" << std::endl;
}
