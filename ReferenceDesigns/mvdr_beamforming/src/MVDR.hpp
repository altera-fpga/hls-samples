#ifndef __MVDR_HPP__
#define __MVDR_HPP__

#include <sycl/sycl.hpp>
#include <sycl/ext/intel/fpga_extensions.hpp>
#include <array>

// utility classes
#include "mvdr_complex.hpp"
#include "pipe_utils.hpp" // Included from include/

// MVDR processing kernels
#include "BackwardSubstitution.hpp"
#include "Beamformer.hpp"
#include "CalcWeights.hpp"
#include "ForwardSubstitution.hpp"
#include "InputDemux.hpp"
#include "SteeringVectorGenerator.hpp"
#include "StreamingQRDWrapper.hpp"
#include "DiagReciprocal.hpp"
#include "Transpose.hpp"

using namespace sycl;

// Names of kernels launched by SubmitMVDRKernels.
// This enum should be used to access elements in the returned vector of events.
enum class MVDRKernelNames {
  input_demux,
  transpose,
  streaming_qrd,
  diag_reciprocal,
  steering_vector_generator,
  forward_substitution,
  backward_substitution,
  calc_weights,
  beamformer,

  // count must come last
  count
};

using MVDREventArray =
    std::array<event, static_cast<int>(MVDRKernelNames::count)>;

// forward declare the names of all kernels to prevent name mangling
template <size_t k_instance_num>
class InputDemux;
template <size_t k_instance_num>
class Transpose;
template <size_t k_instance_num>
class StreamingQRD;
template <size_t k_instance_num>
class DiagReciprocal;
template <size_t k_instance_num>
class SteeringVectorGenerator;
template <size_t k_instance_num>
class ForwardSubstitution;
template <size_t k_instance_num>
class BackwardSubstitution;
template <size_t k_instance_num>
class CalcWeights;
template <size_t k_instance_num>
class Beamformer;

// forward declare the names of all pipes and pipe duplicators to prevent name
// mangling
template <size_t k_instance_num>
class TrainingDataPipeID;
template <size_t k_instance_num>
class XrxDataPipeID;
template <size_t k_instance_num>
class SteeringVectorsPipeID;
template <size_t k_instance_num>
class UpdateSteeringVectorsPipeID;
template <size_t k_instance_num>
class ForwardSteeringVectorsPipeID;
template <size_t k_instance_num>
class QMatrixPipeID;
template <size_t k_instance_num>
class RMatrixPipesID;
template <size_t k_instance_num>
class RDiagRecipVectorPipesID;
template <size_t k_instance_num>
class ForwardSubstitutionResultPipeID;
template <size_t k_instance_num>
class YVectorsPipeID;
template <size_t k_instance_num>
class WeightVectorsPipeID;
template <size_t k_instance_num>
class TransposedTrainingDataPipeID;
template <size_t k_instance_num>
class TrainingDataDupPipeID;
template <size_t k_instance_num>
class XrxDataDupPipeID;
template <size_t k_instance_num>
class SteeringVectorsDupPipeID;
template <size_t k_instance_num>
class ForwardSteeringVectorsDupPipeID;
template <size_t k_instance_num>
class RMatrixDupPipeID;
template <size_t k_instance_num>
class RDiagRecipVectorDupPipeID;
template <size_t k_instance_num>
class ForwardSubstitutionResultDupPipeID;
template <size_t k_instance_num>
class YVectorsDupPipeID;
template <size_t k_instance_num>
class WeightVectorsDupPipeID;
template <size_t k_instance_num>
class TransposedTrainingDataDupPipeID;
class MVDRNullPipeID;

// SubmitMVDRKernels
// Launch all the kernels to perform MVDR processing.
// Return a vector of events, one for each kernel
template <
    size_t k_num_sensor_inputs,     // number of sensor array inputs
    size_t k_rmb_factor,            // Reed-Mallett-Brennan rule
                                    // Number of 'rows' of sensor data used
                                    // by the QRD is k_num_sensor_inputs *
                                    // k_rmb_factor (generally 2-5)
    size_t k_num_steering_vectors,  // number of steering vectors to apply to
                                    // each input sample
    size_t k_subst_unroll_factor,   // unroll factor used by the forward and
                                    // backward substitution kernels
    size_t k_beam_unroll_factor,    // unroll factor used by beamformer
    size_t k_qrd_min_iterations,    // minimum 'inner loop' iterations for the
                                    // QRD kernel, this number can be tuned
                                    // for best throughput
    size_t k_num_complex_per_xrx_read,  // Number of complex numbers (contained
                                        // in NTuple) per read from the
                                        // Xrx input pipes

    typename DataInPipe,        // Sensor data to be processed.  Includes
                                // embedded headers to identify training
                                // and processing data
                                // Accept an NTuple containing
                                // k_num_complex_per_xrx_read complex
                                // floats per read
    typename SinThetaInPipe,    // sin(theta) input for generating
                                // steering vectors. Updated by another
                                // kernel with updates from the host.
                                // Accept one float per read.
    typename DataOutPipe,       // For each Xrx input data vector, send
                                // an output for each of the Weight
                                // vectors.
                                // Send one complex float per write.
    size_t k_instance_num = 0,  // To allow more than one MVDR instance
                                // in a system, provide a unique
                                // instance_num to each.

    // copies of internal pipes, useful for debugging or other processing
    // all default to 'null' pipes that go nowhere
    typename TrainingDataPipeOut =
        fpga_tools::PipeDuplicator<MVDRNullPipeID,
          fpga_tools::NTuple<ComplexType, k_num_complex_per_xrx_read>>,
    typename XrxDataPipeOut =
        fpga_tools::PipeDuplicator<MVDRNullPipeID,
          fpga_tools::NTuple<ComplexType, k_num_complex_per_xrx_read>>,
    typename SteeringVectorsPipeOut =
        fpga_tools::PipeDuplicator<MVDRNullPipeID, ComplexType>,
    typename ForwardSteeringVectorsPipeOut =
        fpga_tools::PipeDuplicator<MVDRNullPipeID, ComplexType>,
    typename RMatrixPipeOut =
        fpga_tools::PipeDuplicator<MVDRNullPipeID, ComplexType>,
    typename RDiagRecipVectorPipeOut =
        fpga_tools::PipeDuplicator<MVDRNullPipeID, float>,
    typename ForwardSubstitutionResultPipeOut =
        fpga_tools::PipeDuplicator<MVDRNullPipeID, ComplexType>,
    typename YVectorsPipeOut =
        fpga_tools::PipeDuplicator<MVDRNullPipeID, ComplexType>,
    typename WeightVectorsPipeOut =
        fpga_tools::PipeDuplicator<MVDRNullPipeID, ComplexType>,
    typename TransposedTrainingDataPipeOut =
        fpga_tools::PipeDuplicator<MVDRNullPipeID,
          fpga_tools::NTuple<ComplexType, k_num_complex_per_xrx_read>>>
MVDREventArray SubmitMVDRKernels(
    queue& q,
    short num_xrx_per_weights  // Number of xrx vectors to process with
                               // each set of Weight vectors.
) {
  constexpr size_t kNumTrainingRows = k_num_sensor_inputs * k_rmb_factor;
  constexpr size_t kTrainingMatrixSize = kNumTrainingRows * k_num_sensor_inputs;

  // Template parameter checking
  // Most template parameters will be checked in individual kernels
  static_assert(k_num_sensor_inputs > 0,
                "k_num_sensor_inputs must be greater than 0");
  static_assert(k_rmb_factor > 0, "k_rmb_factor must be greater than 0");
  static_assert(std::numeric_limits<short>::max() > kNumTrainingRows,
                "k_num_sensor_inputs * k_rmb_factor must fit in a short");

  // Multiple pipes use this type, a group of complex wrapped in an NTuple
  using XrxPipeType = fpga_tools::NTuple<ComplexType, k_num_complex_per_xrx_read>;

  // Training data pipe (after demux from input data)
  constexpr int kTrainingDataPipeMinDepth =
      kTrainingMatrixSize / k_num_complex_per_xrx_read;
  using TrainingDataPipe =
      sycl::ext::intel::pipe<TrainingDataPipeID<k_instance_num>, XrxPipeType,
                        kTrainingDataPipeMinDepth>;
  using TrainingDataDupPipe =
      fpga_tools::PipeDuplicator<TrainingDataDupPipeID<k_instance_num>,
                                 XrxPipeType, TrainingDataPipe,
                                 TrainingDataPipeOut>;

  // Xrx processing data pipe and duplicator (after demux from input data)
  // Must provide sufficient depth to not produce backpressure while training
  // data is processed (4 full matrices is adequate)                     
  constexpr int kXrxDataPipeMinDepth =
      (kTrainingMatrixSize / k_num_complex_per_xrx_read) * 4;
  using XrxDataPipe = sycl::ext::intel::pipe<XrxDataPipeID<k_instance_num>,
                                        XrxPipeType, kXrxDataPipeMinDepth>;

  // Steering vector generator pipe and duplicator, and related update pipe
  // Connect SteeringVectorGenerator to ForwardSubstitution
  constexpr int kSteeringVectorsPipeMinDepth =
      k_num_steering_vectors * k_num_sensor_inputs * 2;
  using SteeringVectorsPipe =
      sycl::ext::intel::pipe<SteeringVectorsPipeID<k_instance_num>, ComplexType,
                        kSteeringVectorsPipeMinDepth>;
  using SteeringVectorsDupPipe =
      fpga_tools::PipeDuplicator<SteeringVectorsDupPipeID<k_instance_num>,
                                 ComplexType, SteeringVectorsPipe,
                                 SteeringVectorsPipeOut>;
  using UpdateSteeringVectorsPipe =
      sycl::ext::intel::pipe<UpdateSteeringVectorsPipeID<k_instance_num>, bool, 1>;

  // Pipe for forwarding steering vectors used by ForwardSubstitution to
  // CalcWeights and pipe duplicator
  using ForwardSteeringVectorsPipe =
      sycl::ext::intel::pipe<ForwardSteeringVectorsPipeID<k_instance_num>,
                        ComplexType, kSteeringVectorsPipeMinDepth>;
  using ForwardSteeringVectorsDupPipe =
      fpga_tools::PipeDuplicator<
          ForwardSteeringVectorsDupPipeID<k_instance_num>, ComplexType,
          ForwardSteeringVectorsPipe, ForwardSteeringVectorsPipeOut>;

  // R matrix and R matrix reciprocal diagonal entries pipes and duplicator
  // Connect StreamingQRD to ForwardSubstitution and BackwardSubstitution
  // Need two copies of each so create 1D arrays of Pipes
  // Min depth ensures we can hold 2 full R matricies in the pipe, to make sure
  // pipe feeding BackwardSubstitution won't overflow while waiting for result
  // from ForwardSubstitution.
  constexpr int kRMatrixPipeMinDepth =
      ((k_num_sensor_inputs * (k_num_sensor_inputs + 1)) / 2) * 2;
  using RMatrixPipes =
      fpga_tools::PipeArray<RMatrixPipesID<k_instance_num>, ComplexType,
                            kRMatrixPipeMinDepth, 3>;
  using RMatrixFSPipe = typename RMatrixPipes::template PipeAt<0>;
  using RMatrixBSPipe = typename RMatrixPipes::template PipeAt<1>;
  using RMatrixDRPipe = typename RMatrixPipes::template PipeAt<2>;
  using RMatrixDupPipe =
      fpga_tools::PipeDuplicator<RMatrixDupPipeID<k_instance_num>, ComplexType,
                                 RMatrixFSPipe, RMatrixBSPipe, RMatrixDRPipe, RMatrixPipeOut>;
  constexpr int kRDiagRecipVectorPipeMinDepth = k_num_sensor_inputs * 2;
  using RDiagRecipVectorPipes =
      fpga_tools::PipeArray<RDiagRecipVectorPipesID<k_instance_num>, float,
                            kRDiagRecipVectorPipeMinDepth, 2>;
  using RDiagRecipVectorFSPipe =
      typename RDiagRecipVectorPipes::template PipeAt<0>;
  using RDiagRecipVectorBSPipe =
      typename RDiagRecipVectorPipes::template PipeAt<1>;
  using RDiagRecipVectorDupPipe =
      fpga_tools::PipeDuplicator<RDiagRecipVectorDupPipeID<k_instance_num>,
                                 float, RDiagRecipVectorFSPipe,
                                 RDiagRecipVectorBSPipe,
                                 RDiagRecipVectorPipeOut>;

  // Forward substitution result pipe and duplicator
  // Connect ForwardSubstitution to BackwardSubstitution
  using ForwardSubstitutionResultPipe =
      sycl::ext::intel::pipe<ForwardSubstitutionResultPipeID<k_instance_num>,
                        ComplexType, k_num_sensor_inputs>;
  using ForwardSubstitutionResultDupPipe =
      fpga_tools::PipeDuplicator<
          ForwardSubstitutionResultDupPipeID<k_instance_num>, ComplexType,
          ForwardSubstitutionResultPipe, ForwardSubstitutionResultPipeOut>;

  // Y vectors pipe
  // Y = (inverse(R x Rtranspose) ) * (complex_conjugate(C)) , where
  // R is the R matrix from QRD, and C is the steering vector
  // Connect BackwardSubstitution to CalcWeights
  using YVectorsPipe = sycl::ext::intel::pipe<YVectorsPipeID<k_instance_num>,
                                         ComplexType, k_num_sensor_inputs>;
  using YVectorsDupPipe =
      fpga_tools::PipeDuplicator<YVectorsDupPipeID<k_instance_num>,
                                 ComplexType, YVectorsPipe, YVectorsPipeOut>;

  // Weight vectors pipe
  // Connect CalcWeights to Beamformer
  constexpr int kWeightVectorsPipeMinDepth =
      k_num_steering_vectors * k_num_sensor_inputs * 2;
  using WeightVectorsPipe =
      sycl::ext::intel::pipe<WeightVectorsPipeID<k_instance_num>, ComplexType,
                        kWeightVectorsPipeMinDepth>;
  using WeightVectorsDupPipe =
      fpga_tools::PipeDuplicator<WeightVectorsDupPipeID<k_instance_num>,
                                 ComplexType, WeightVectorsPipe,
                                 WeightVectorsPipeOut>;

  // Q matrix pipe
  // Q matrix not used in MVDR design, so this is a 'null' pipe (a
  // PipeDuplicator with no output pipes connected)
  using QMatrixColumn = fpga_tools::NTuple<ComplexType, k_num_complex_per_xrx_read>;
  using QMatrixPipe =
      fpga_tools::PipeDuplicator<QMatrixPipeID<k_instance_num>, QMatrixColumn>;

  // transposed training data pipe
  constexpr int kTransposedTrainingDataPipeMinDepth = kTrainingDataPipeMinDepth;
  using TransposedTrainingDataPipe =
      sycl::ext::intel::pipe<TransposedTrainingDataPipeID<k_instance_num>,
                        XrxPipeType, kTransposedTrainingDataPipeMinDepth>;
  using TransposedTrainingDataDupPipe =
     fpga_tools:: PipeDuplicator<
        TransposedTrainingDataDupPipeID<k_instance_num>, XrxPipeType,
        TransposedTrainingDataPipe, TransposedTrainingDataPipeOut>;

  // array of events to return
  // use MVDRKernelNames enum as indicies into the array
  MVDREventArray events;

  events[static_cast<int>(MVDRKernelNames::input_demux)] =
      SubmitInputDemuxKernel<
          InputDemux<k_instance_num>,  // Name to use for the Kernel
          k_num_complex_per_xrx_read,  // Number of elements per pipe read/write
          kTrainingMatrixSize,         // Complex numbers per training matrix
          kTrainingMatrixSize,  // maximum number of complex numbers in a
                                // set of xrx data to be matched with each
                                // training matrix to support
          false,                // read every cycle (true) or only when
                                // space is available (false)
          DataInPipe,           // Incoming data, including headers
          TrainingDataDupPipe,  // Send training data to QRD
          XrxDataPipe           // Send sample data to Beamformer
          >(q, (int)num_xrx_per_weights * (int)k_num_sensor_inputs);

  events[static_cast<int>(MVDRKernelNames::steering_vector_generator)] =
      SubmitSteeringVectorGeneratorKernel<
          SteeringVectorGenerator<k_instance_num>,  // Name to use for the
                                                    // Kernel
          k_num_steering_vectors,    // number of steering vectors
          k_num_sensor_inputs,       // number of elements in each vector
          SinThetaInPipe,            // sin(theta) input
          SteeringVectorsDupPipe,    // generated steering vectors
          UpdateSteeringVectorsPipe  // load new steering vectors
          >(q);

  events[static_cast<int>(MVDRKernelNames::transpose)] = SubmitTransposeKernel<
      Transpose<k_instance_num>,     // Name to use for the Kernel
      ComplexType,                   // type of element to transpose
      k_num_sensor_inputs,           // number of columns in the input matrix
      k_num_complex_per_xrx_read,    // number of elements per pipe read/write
      TrainingDataPipe,              // training matrix input
      TransposedTrainingDataDupPipe  // Output matrix
      >(q);

  events[static_cast<int>(MVDRKernelNames::streaming_qrd)] =
      SubmitStreamingQRDKernel<
          StreamingQRD<k_instance_num>,  // Name to use for the Kernel
          k_qrd_min_iterations,  // Minimum number of inner loop iterations
          kNumTrainingRows,      // Number of rows in the incoming A matrix
          k_num_sensor_inputs,   // Number of columns in the incoming A matrix
          k_num_complex_per_xrx_read,  // number of elements per pipe read
          TransposedTrainingDataPipe,  // A matrix input
          QMatrixPipe,                 // Q output pipe (unused in MVDR)
          RMatrixDupPipe               // R output pipe
          >(q);

  events[static_cast<int>(MVDRKernelNames::diag_reciprocal)] =
      SubmitDiagReciprocalKernel<
          DiagReciprocal<k_instance_num>,  // Name to use for the Kernel
          k_num_sensor_inputs,             // number of rows of the R matrix
          RMatrixDRPipe,                   // Input R pipe
          RDiagRecipVectorDupPipe          // Output pipe for reciprocals
          >(q);

  events[static_cast<int>(MVDRKernelNames::forward_substitution)] =
      SubmitForwardSubstitutionKernel<
          ForwardSubstitution<k_instance_num>,  // Name to use for the Kernel
          k_num_sensor_inputs,            // Number of elements in each vector
          k_subst_unroll_factor,          // inner loop unroll factor
          k_num_steering_vectors,         // Number of y vectors
          RMatrixFSPipe,                  // lower-triangular matrix L
          RDiagRecipVectorFSPipe,         // 1 / diag of L
          SteeringVectorsPipe,            // Y vectors in
          UpdateSteeringVectorsPipe,      // load new Y vectors
          ForwardSteeringVectorsDupPipe,  // Steering vectors used to calculate
                                          // X
          ForwardSubstitutionResultDupPipe  // X vectors out
          >(q);

  events[static_cast<int>(MVDRKernelNames::backward_substitution)] =
      SubmitBackwardSubstitutionKernel<
          BackwardSubstitution<k_instance_num>,  // Name to use for the Kernel
          k_num_sensor_inputs,            // Number of elements in each vector
          k_subst_unroll_factor,          // inner loop unroll factor
          k_num_steering_vectors,         // Number of y vectors
          RMatrixBSPipe,                  // upper-triangular matrix U.
          RDiagRecipVectorBSPipe,         // 1 / diag of U
          ForwardSubstitutionResultPipe,  // Y vectors in
          YVectorsDupPipe                 // X vectors out
          >(q);

  events[static_cast<int>(MVDRKernelNames::calc_weights)] =
      SubmitCalcWeightsKernel<
          CalcWeights<k_instance_num>,  // Name to use for the Kernel
          k_num_steering_vectors,       // number of steering vectors
          k_num_sensor_inputs,          // number of elements in each vector
          YVectorsPipe,                 // Receive the Y vectors.
          ForwardSteeringVectorsPipe,   // steering vectors
          WeightVectorsDupPipe          // weight vectors output
          >(q);

  events[static_cast<int>(MVDRKernelNames::beamformer)] =
      SubmitBeamformerKernel<
          Beamformer<k_instance_num>,  // Name to use for the Kernel
          k_num_steering_vectors,      // number of weight vectors
          k_num_sensor_inputs,         // number of elements in each vector
          k_num_complex_per_xrx_read,  // complex numbers per xrx pipe read
          k_beam_unroll_factor,        // unroll factor
          XrxDataPipe,                 // Receive the Xrx vectors
          WeightVectorsPipe,           // weight vectors input
          DataOutPipe                  // final data output
          >(q, num_xrx_per_weights);

  return events;

}  // end of SubmitMVDRKernels()

#endif  // ifndef __MVDR_HPP__
