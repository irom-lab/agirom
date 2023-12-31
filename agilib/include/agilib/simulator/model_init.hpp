#pragma once

#include "agilib/math/math.hpp"
#include "agilib/simulator/model_base.hpp"

namespace agi {

class ModelInit : public ModelBase {
 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  ModelInit(Quadrotor quad) : ModelBase(quad) {}
  bool run(const Ref<const Vector<QS::SIZE>>,
           Ref<Vector<QS::SIZE>>) const override;
};


}  // namespace agi
