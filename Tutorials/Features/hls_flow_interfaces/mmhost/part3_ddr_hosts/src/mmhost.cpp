#include <sycl/ext/altera/fpga_extensions.hpp>
#include <sycl/sycl.hpp>

#include "exception_handler.hpp"

namespace oneapi_exp = sycl::ext::oneapi::experimental;
namespace altera_exp = sycl::ext::altera::experimental;

constexpr int kBL1 = 1;
constexpr int kBL2 = 2;
constexpr int kAlignment = 32;

struct DDRIP {
  using ParamsBl1 = decltype(oneapi_exp::properties{
      altera_exp::buffer_location<kBL1>,
      altera_exp::awidth<32>,
      altera_exp::dwidth<256>,
      altera_exp::latency<0>,
      altera_exp::maxburst<8>,
      oneapi_exp::alignment<kAlignment>});

  using ParamsBl2 = decltype(oneapi_exp::properties{
      altera_exp::buffer_location<kBL2>,
      altera_exp::awidth<32>,
      altera_exp::dwidth<256>,
      altera_exp::latency<0>,
      altera_exp::maxburst<8>,
      oneapi_exp::alignment<kAlignment>});

  oneapi_exp::annotated_arg<int *, ParamsBl1> x;
  oneapi_exp::annotated_arg<int *, ParamsBl1> y;
  oneapi_exp::annotated_arg<int *, ParamsBl2> z;
  int size;

  void operator()() const {
#pragma unroll 8
    for (int i = 0; i < size; ++i) {
      z[i] = x[i] + y[i];
    }
  }
};

int main(void) {
#if FPGA_SIMULATOR
  auto selector = sycl::ext::altera::fpga_simulator_selector_v;
#elif FPGA_HARDWARE
  auto selector = sycl::ext::altera::fpga_selector_v;
#else  // #if FPGA_EMULATOR
  auto selector = sycl::ext::altera::fpga_emulator_selector_v;
#endif

  bool passed = true;

  try {
    // create the device queue
    sycl::queue q(selector, fpga_tools::exception_handler);

    // Print out the device information.
    sycl::device device = q.get_device();
    std::cout << "Running on device: "
              << q.get_device().get_info<sycl::info::device::name>().c_str()
              << std::endl;

    // Create and initialize the host arrays
    constexpr int kN = 8;
    std::cout << "Elements in vector : " << kN << "\n";

    // Host array must share the same buffer location property as defined in
    // the kernel. Since we are specifying alignment on the kernel argument,
    // we need to also specify that to the allocation call by using
    // aligned_alloc_shared API
    int *array_a = sycl::aligned_alloc_shared<int>(
        kAlignment, kN, q,
        altera_exp::property::usm::buffer_location(kBL1));
    int *array_b = sycl::aligned_alloc_shared<int>(
        kAlignment, kN, q,
        altera_exp::property::usm::buffer_location(kBL1));
    int *array_c = sycl::aligned_alloc_shared<int>(
        kAlignment, kN, q,
        altera_exp::property::usm::buffer_location(kBL2));

    assert(array_a);
    assert(array_b);
    assert(array_c);

    for (int i = 0; i < kN; i++) {
      array_a[i] = i;
      array_b[i] = 2 * i;
      array_c[i] = 0;
    }

    // Launch the kernel
    q.single_task(DDRIP{array_a, array_b, array_c, kN}).wait();

    // Verify the results
    for (int i = 0; i < kN; i++) {
      auto golden = 3 * i;
      if (array_c[i] != golden) {
        std::cout << "ERROR! At index: " << i << " , expected: " << golden
                  << " , found: " << array_c[i] << "\n";
        passed = false;
      }
    }

    sycl::free(array_a, q);
    sycl::free(array_b, q);
    sycl::free(array_c, q);

  } catch (sycl::exception const &e) {
    // Catches exceptions in the host code
    std::cerr << "Caught a SYCL host exception:\n" << e.what() << "\n";

    // Most likely the runtime couldn't find FPGA hardware!
    if (e.code().value() == CL_DEVICE_NOT_FOUND) {
      std::cerr << "If you are targeting the FPGA emulator, compile with "
                   "-DFPGA_EMULATOR.\n";
    }
    std::terminate();
  }

  std::cout << (passed ? "PASSED" : "FAILED") << std::endl;

  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
