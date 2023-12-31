#include "agilib/simulator/quadrotor_simulator.hpp"

#include "agilib/math/gravity.hpp"

namespace agi {

QuadrotorSimulator::QuadrotorSimulator(const Quadrotor &quadrotor)
  : QuadrotorSimulator(SimulatorParams(quadrotor)) {}

QuadrotorSimulator::QuadrotorSimulator(const SimulatorParams &params)
  : SimulatorBase("Quadrotor Simulator"), params_(params) {
  params_.createLowLevelController(ctrl_);
  params_.createModelPipeline(model_pipeline_);
  params_.createIntegrator(integrator_ptr_, this->getDynamics());

  reset();
}

bool QuadrotorSimulator::run(const Command &cmd, const Scalar ctl_dt) {
  if (!setCommand(cmd)) return false;
  return run(ctl_dt);
}

bool QuadrotorSimulator::run(const Scalar ctl_dt) {
  if (!state_.valid()) return false;

  QuadState next_state = state_;
  const Scalar max_dt = integrator_ptr_->dtMax();
  Scalar remain_ctl_dt = ctl_dt;

  // simulation loop
  while (remain_ctl_dt > 0.0) {
    const Scalar sim_dt = std::min(remain_ctl_dt, max_dt);
    // check if we need to update the reference command for the controller
    if (!cmd_queue_.empty() &&
        (state_.t >= cmd_queue_.front().t + params_.delay_)) {
      if (!ctrl_->setCommand(cmd_queue_.front())) {
        logger_.error("Could not set command.");
        return false;
      }
      cmd_queue_.pop();
    }
    ctrl_->setState(state_);
    ctrl_->getMotorCommand(state_.motdes);

    if (!std::isfinite(params_.quadrotor_.motor_tau_inv_)) {
      state_.mot = state_.motdes;
    }

    integrator_ptr_->step(state_.x, sim_dt, next_state.x);
    updateState(next_state, sim_dt);

    if (state_.p.z() <= 0.0) {
      state_.p.z() = 0.0;
      state_.v.z() = 0.0;
      state_.a.z() = 0.0;
    }

    remain_ctl_dt -= sim_dt;
  }

  state_.qx.normalize();

  state_.t += ctl_dt;
  return true;
}

bool QuadrotorSimulator::reset(const bool &reset_time) {
  state_.setZero(reset_time);
  while (!cmd_queue_.empty()) cmd_queue_.pop();
  return true;
}

bool QuadrotorSimulator::reset(const QuadState &state) {
  if (!state.valid()) return false;
  state_ = state;
  while (!cmd_queue_.empty()) cmd_queue_.pop();
  return true;
}

bool QuadrotorSimulator::computeDynamics(
  const Ref<const Vector<QS::SIZE>> state,
  Ref<Vector<QS::SIZE>> derivative) const {
  for (auto &m : model_pipeline_) {
    m->run(state, derivative);
  }
  return true;
}

void QuadrotorSimulator::updateState(const QuadState &next_state,
                                     Scalar sim_dt) {
  // Calculate mean acceleration and body torque over time step
  const Vector<3> delta_vel = next_state.v - state_.v;

  state_ = next_state;
  state_.a = delta_vel / sim_dt;
}

bool QuadrotorSimulator::setCommand(const Command &cmd) {
  cmd_queue_.push(cmd);
  return true;
}

bool QuadrotorSimulator::setState(const QuadState &state) {
  if (!state.valid()) return false;
  state_ = state;
  return true;
}

bool QuadrotorSimulator::getState(QuadState *const state) const {
  if (!state_.valid()) return false;
  *state = state_;
  return true;
}

bool QuadrotorSimulator::getQuadrotor(Quadrotor *const quad) const {
  if (quad == nullptr || !params_.quadrotor_.valid()) return false;
  *quad = params_.quadrotor_;
  return true;
}

const Quadrotor &QuadrotorSimulator::getQuadrotor() const {
  return params_.quadrotor_;
}

const std::shared_ptr<LowLevelControllerBase>
QuadrotorSimulator::getLowLevelController() const {
  return ctrl_;
}

DynamicsFunction QuadrotorSimulator::getDynamics() const {
  return std::bind(
    static_cast<bool (QuadrotorSimulator::*)(const Ref<const Vector<QS::SIZE>>,
                                             Ref<Vector<QS::SIZE>>) const>(
      &QuadrotorSimulator::computeDynamics),
    this, std::placeholders::_1, std::placeholders::_2);
}

bool QuadrotorSimulator::updateQuad(const Quadrotor &quad) {
  if (!quad.valid()) {
    return false;
  }
  params_.quadrotor_ = quad;
  ctrl_->updateQuad(quad);

  for (auto &m : model_pipeline_) m->updateQuad(quad);
  params_.createIntegrator(integrator_ptr_, this->getDynamics());

  return true;
}

}  // namespace agi
