#pragma once

#include <cmath>
#include <mutex>
#include <thread>

#include "agilib/controller/controller_base.hpp"
#include "agilib/controller/pid/pid_params.hpp"
#include "agilib/math/gravity.hpp"
#include "agilib/math/math.hpp"
#include "agilib/math/types.hpp"
#include "agilib/types/command.hpp"
#include "agilib/types/quad_state.hpp"
#include "agilib/types/quadrotor.hpp"
#include "agilib/utils/logger.hpp"
#include "agilib/utils/low_pass_filter.hpp"
#include "agilib/utils/timer.hpp"

namespace agi {


class PidController : public ControllerBase {
 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  PidController(const Quadrotor& quad,
                const std::shared_ptr<PidParameters>& params);
  ~PidController();

  virtual bool getCommand(const QuadState& state,
                          const SetpointVector& references,
                          SetpointVector* const setpoints) override;

  bool reset();
  bool reset(const QuadState& state);

  bool updateParameters(const Quadrotor& quad,
                        const std::shared_ptr<PidParameters> params);
  bool updateParameters(const Quadrotor& params);
  bool updateParameters(const std::shared_ptr<PidParameters> params);
  std::shared_ptr<PidParameters> getParameters();
  //   void addImuSample(const ImuSample& sample) override {}

 private:
  Quadrotor quad_;
  std::shared_ptr<PidParameters> params_;
  LowPassFilter<3> filterGyr_;
  Scalar t_last_;
  Vector<3> omega_err_int_;
  Matrix<> G_inv_;  // We know beforehand it's 4x4 matrix, but we need to do
                    // it dynamically allocated because the closed form
                    // inversion of a 4x4 matrix in Eigen seems broken
};

}  // namespace agi
