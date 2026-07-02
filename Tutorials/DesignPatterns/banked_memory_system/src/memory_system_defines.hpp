#pragma once

#include <array>

#include <sycl/ext/altera/experimental/pipes_ext.hpp>
#include <sycl/ext/altera/fpga_extensions.hpp>
#include <sycl/sycl.hpp>

#include "constexpr_math.hpp"

namespace altera_exp = sycl::ext::altera::experimental;
namespace oneapi_exp = sycl::ext::oneapi::experimental;

// Constants for the simple kernels.
constexpr size_t kNumRows = 5;
constexpr size_t kNumCols = 500;

constexpr size_t kNumRowsOptimized = 500;

using SimpleInputT = int;
using SimpleOutputT = std::array<int, 5>;

/////////////////////////////////////////////
// Define input/output streaming interfaces
/////////////////////////////////////////////
using PipePropertiesT = decltype(oneapi_exp::properties(
    altera_exp::bits_per_symbol<8>,
    altera_exp::uses_valid<true>,
    altera_exp::ready_latency<0>,
    altera_exp::first_symbol_in_high_order_bits<true>));

// Interfaces for the illustrative simple kernels.
class IDInStreamNaiveKernel;
using InStreamNaiveKernel =
    altera_exp::pipe<IDInStreamNaiveKernel, SimpleInputT, 0, PipePropertiesT>;

class IDOutStreamNaiveKernel;
using OutStreamNaiveKernel =
    altera_exp::pipe<IDOutStreamNaiveKernel, SimpleOutputT, 0, PipePropertiesT>;

class IDInStreamOptKernel;
using InStreamOptKernel =
    altera_exp::pipe<IDInStreamOptKernel, SimpleInputT, 0, PipePropertiesT>;
class IDOutStreamOptKernel;
using OutStreamOptKernel =
    altera_exp::pipe<IDOutStreamOptKernel, SimpleOutputT, 0, PipePropertiesT>;
