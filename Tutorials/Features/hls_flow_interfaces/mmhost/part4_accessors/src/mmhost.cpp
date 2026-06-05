#include <sycl/ext/altera/fpga_extensions.hpp>
#include <sycl/sycl.hpp>

#include "exception_handler.hpp"

constexpr int kBL1 = 1;
constexpr int kBL2 = 2;
constexpr int kBL3 = 3;

template <class AccA, class AccB, class AccC>
struct AccessorMMIP {
  AccA x;
  AccB y;
  AccC z;
  int size;

  void operator()() const {
    for (int i = 0; i < size; ++i) {
      z[i] = x[i] + y[i];
    }
  }
};

template <int N>
void RunKernel(sycl::queue &q, std::array<int, N> &array_a,
               std::array<int, N> &array_b, std::array<int, N> &array_c) {
  sycl::buffer buf_a{array_a};
  sycl::buffer buf_b{array_b};
  sycl::buffer buf_c{array_c};

  q.submit([&](sycl::handler &h) {
    sycl::accessor acc_a{buf_a, h, sycl::read_only,
                         sycl::ext::oneapi::accessor_property_list{
                             sycl::ext::altera::buffer_location<kBL1>}};
    sycl::accessor acc_b{buf_b, h, sycl::read_only,
                         sycl::ext::oneapi::accessor_property_list{
                             sycl::ext::altera::buffer_location<kBL2>}};
    sycl::accessor acc_c{buf_c, h, sycl::write_only,
                         sycl::ext::oneapi::accessor_property_list{
                             sycl::ext::altera::buffer_location<kBL3>,
                             sycl::no_init}};

    h.single_task(
        AccessorMMIP<decltype(acc_a), decltype(acc_b), decltype(acc_c)>{
            acc_a, acc_b, acc_c, N});
  });
}

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

    std::array<int, kN> array_a, array_b, array_c;
    for (int i = 0; i < kN; i++) {
      array_a[i] = i;
      array_b[i] = 2 * i;
      array_c[i] = 0;
    }

    // Launch the kernel
    RunKernel<kN>(q, array_a, array_b, array_c);

    // Verify the results
    for (int i = 0; i < kN; i++) {
      auto golden = 3 * i;
      if (array_c[i] != golden) {
        std::cout << "ERROR! At index: " << i << " , expected: " << golden
                  << " , found: " << array_c[i] << "\n";
        passed = false;
      }
    }

  } catch (sycl::exception const &e) {
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

  std::cout << (passed ? "PASSED" : "FAILED") << std::endl;

  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
