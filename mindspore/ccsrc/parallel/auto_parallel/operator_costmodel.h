/**
 * Copyright 2019 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PARALLEL_AUTO_PARALLEL_OPERATOR_COSTMODEL_H_
#define PARALLEL_AUTO_PARALLEL_OPERATOR_COSTMODEL_H_

#include <vector>
#include <memory>
#include "parallel/tensor_layout/tensor_info.h"
#include "parallel/device_manager.h"

namespace mindspore {
namespace parallel {
#define MAXIMUM_INPUT_NUMBER 100
#define DEFAULT_DATA_TYPE_LENGTH 4

class OperatorCost;
using OperatorCostPtr = std::shared_ptr<OperatorCost>;

template <typename T>
double ListProduct(std::vector<T> vec) {
  double result = 1;
  for (size_t i = 0; i < vec.size(); ++i) {
    result *= vec[i];
  }
  return result;
}
// NOTE: Currently, the returned value in each method is bytes of memory size, which is calculated by the number of
// entries timing the length of each entry's data type
class OperatorCost {
 public:
  OperatorCost() {
    // this is only for the case when set_is_parameter() and SetInputAndOutputTypeLength() are not invoked
    for (size_t i = 0; i < MAXIMUM_INPUT_NUMBER; ++i) {
      is_parameter_.push_back(false);
      inputs_type_lengths_.push_back(DEFAULT_DATA_TYPE_LENGTH);
      outputs_type_lengths_.push_back(DEFAULT_DATA_TYPE_LENGTH);
    }
  }
  virtual ~OperatorCost() = default;

  void set_is_parameter(const std::vector<bool>& is_parameter);
  void SetInputAndOutputTypeLength(const std::vector<size_t>& input_lengths, const std::vector<size_t>& output_lengths);
  std::vector<size_t> inputs_type_lengths() const { return inputs_type_lengths_; }
  std::vector<size_t> outputs_type_lengths() const { return outputs_type_lengths_; }

  // per device communication cost
  virtual double GetCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                             const int32_t& stage_id) const = 0;
  virtual double GetForwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                                    const int32_t& stage_id) const = 0;
  virtual double GetBackwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                                     const int32_t& stage_id) const = 0;
  // per device computation cost
  virtual double GetMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                               const int32_t& stage_id) const = 0;
  virtual double GetForwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                                      const int32_t& stage_id) const = 0;
  virtual double GetBackwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                                       const int32_t& stage_id) const = 0;

 protected:
  // for each input in 'inputs_', there is a bool variable indicating whether that the corresponding input is parameter
  std::vector<bool> is_parameter_;
  // for each input and output, the followings record the number of bytes of each element
  std::vector<size_t> inputs_type_lengths_;
  std::vector<size_t> outputs_type_lengths_;
};

class MatMulCost : public OperatorCost {
 public:
  MatMulCost() = default;
  ~MatMulCost() override = default;

  // per device communication cost
  double GetCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                     const int32_t& stage_id) const override {
    return GetForwardCommCost(inputs, outputs, stage_id) + GetBackwardCommCost(inputs, outputs, stage_id);
  }
  double GetForwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                            const int32_t& stage_id) const override;
  double GetBackwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                             const int32_t& stage_id) const override;

  // per device computation cost
  double GetMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                       const int32_t& stage_id) const override {
    return GetForwardMemoryCost(inputs, outputs, stage_id) + GetBackwardMemoryCost(inputs, outputs, stage_id);
  }
  double GetForwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                              const int32_t& stage_id) const override;
  double GetBackwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                               const int32_t& stage_id) const override;
};

using MatMulCostPtr = std::shared_ptr<MatMulCost>;

class ActivationCost : public OperatorCost {
 public:
  ActivationCost() = default;
  ~ActivationCost() override = default;

  double GetCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                     const int32_t& stage_id) const override {
    return GetForwardCommCost(inputs, outputs, stage_id) + GetBackwardCommCost(inputs, outputs, stage_id);
  }
  double GetForwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                            const int32_t& stage_id) const override;
  double GetBackwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                             const int32_t& stage_id) const override;
  double GetMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                       const int32_t& stage_id) const override {
    return GetForwardMemoryCost(inputs, outputs, stage_id) + GetBackwardMemoryCost(inputs, outputs, stage_id);
  }
  double GetForwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                              const int32_t& stage_id) const override;
  double GetBackwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                               const int32_t& stage_id) const override;
};

using ActivationCostPtr = std::shared_ptr<ActivationCost>;

class SoftmaxCost : public OperatorCost {
 public:
  SoftmaxCost() = default;
  ~SoftmaxCost() override = default;

  double GetCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                     const int32_t& stage_id) const override {
    return GetForwardCommCost(inputs, outputs, stage_id) + GetBackwardCommCost(inputs, outputs, stage_id);
  }
  double GetForwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                            const int32_t& stage_id) const override;
  double GetBackwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                             const int32_t& stage_id) const override;
  double GetMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                       const int32_t& stage_id) const override {
    return GetForwardMemoryCost(inputs, outputs, stage_id) + GetBackwardMemoryCost(inputs, outputs, stage_id);
  }
  double GetForwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                              const int32_t& stage_id) const override;
  double GetBackwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                               const int32_t&) const override;
};

using SoftmaxCostPtr = std::shared_ptr<SoftmaxCost>;

class TmpIdentityCost : public OperatorCost {
 public:
  TmpIdentityCost() = default;
  ~TmpIdentityCost() override = default;

  double GetCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                     const int32_t& stage_id) const override {
    return GetForwardCommCost(inputs, outputs, stage_id) + GetBackwardCommCost(inputs, outputs, stage_id);
  }
  double GetForwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                            const int32_t& stage_id) const override;
  double GetBackwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                             const int32_t& stage_id) const override;
  double GetMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                       const int32_t& stage_id) const override {
    return GetForwardMemoryCost(inputs, outputs, stage_id) + GetBackwardMemoryCost(inputs, outputs, stage_id);
  }
  double GetForwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                              const int32_t& stage_id) const override;
  double GetBackwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                               const int32_t& stage_id) const override;
};
using TmpIdentityCostPtr = std::shared_ptr<TmpIdentityCost>;

class BatchParallelCost : public OperatorCost {
 public:
  BatchParallelCost() = default;
  ~BatchParallelCost() override = default;

  double GetCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                     const int32_t& stage_id) const override {
    return GetForwardCommCost(inputs, outputs, stage_id) + GetBackwardCommCost(inputs, outputs, stage_id);
  }
  double GetForwardCommCost(const std::vector<TensorInfo>&, const std::vector<TensorInfo>&,
                            const int32_t&) const override {
    return 0.0;
  }
  double GetBackwardCommCost(const std::vector<TensorInfo>&, const std::vector<TensorInfo>&,
                             const int32_t&) const override {
    return 0.0;
  }
  double GetMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                       const int32_t& stage_id) const override {
    return GetForwardMemoryCost(inputs, outputs, stage_id) + GetBackwardMemoryCost(inputs, outputs, stage_id);
  }
  double GetForwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                              const int32_t& stage_id) const override;
  double GetBackwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                               const int32_t& stage_id) const override;
};
using BatchParallelCostPtr = std::shared_ptr<BatchParallelCost>;

class VirtualDatasetCost : public OperatorCost {
 public:
  VirtualDatasetCost() = default;
  ~VirtualDatasetCost() override = default;

  double GetCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                     const int32_t& stage_id) const override {
    return GetForwardCommCost(inputs, outputs, stage_id) + GetBackwardCommCost(inputs, outputs, stage_id);
  }
  double GetForwardCommCost(const std::vector<TensorInfo>&, const std::vector<TensorInfo>&,
                            const int32_t&) const override {
    return 0.0;
  }
  double GetBackwardCommCost(const std::vector<TensorInfo>&, const std::vector<TensorInfo>&,
                             const int32_t&) const override {
    return 0.0;
  }
  double GetMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                       const int32_t& stage_id) const override {
    return GetForwardMemoryCost(inputs, outputs, stage_id) + GetBackwardMemoryCost(inputs, outputs, stage_id);
  }
  double GetForwardMemoryCost(const std::vector<TensorInfo>&, const std::vector<TensorInfo>&,
                              const int32_t&) const override {
    return 0.0;
  }
  double GetBackwardMemoryCost(const std::vector<TensorInfo>&, const std::vector<TensorInfo>&,
                               const int32_t&) const override {
    return 0.0;
  }
};
using VirtualDatasetCostPtr = std::shared_ptr<VirtualDatasetCost>;

class GeneratorBaseCost : public OperatorCost {
 public:
  GeneratorBaseCost() = default;
  ~GeneratorBaseCost() override = default;

  double GetCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                     const int32_t& stage_id) const override {
    return GetForwardCommCost(inputs, outputs, stage_id) + GetBackwardCommCost(inputs, outputs, stage_id);
  }
  double GetForwardCommCost(const std::vector<TensorInfo>&, const std::vector<TensorInfo>&,
                            const int32_t&) const override {
    return 0.0;
  }
  double GetBackwardCommCost(const std::vector<TensorInfo>&, const std::vector<TensorInfo>&,
                             const int32_t&) const override {
    return 0.0;
  }
  double GetMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                       const int32_t& stage_id) const override {
    return GetForwardMemoryCost(inputs, outputs, stage_id) + GetBackwardMemoryCost(inputs, outputs, stage_id);
  }
  // Inputs vector is empty for generator ops.
  double GetForwardMemoryCost(const std::vector<TensorInfo>&, const std::vector<TensorInfo>&,
                              const int32_t&) const override {
    return 0.0;
  }
  // Generator ops don't have backward steps.
  double GetBackwardMemoryCost(const std::vector<TensorInfo>&, const std::vector<TensorInfo>&,
                               const int32_t&) const override {
    return 0.0;
  }
};
using GeneratorBaseCostPtr = std::shared_ptr<GeneratorBaseCost>;

class PReLUCost : public OperatorCost {
 public:
  PReLUCost() = default;
  ~PReLUCost() override = default;

  // per device communication cost
  double GetCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                     const int32_t& stage_id) const override {
    return GetForwardCommCost(inputs, outputs, stage_id) + GetBackwardCommCost(inputs, outputs, stage_id);
  }
  double GetForwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                            const int32_t& stage_id) const override;
  double GetBackwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                             const int32_t& stage_id) const override;

  // per device computation cost
  double GetMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                       const int32_t& stage_id) const override {
    return GetForwardMemoryCost(inputs, outputs, stage_id) + GetBackwardMemoryCost(inputs, outputs, stage_id);
  }
  double GetForwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                              const int32_t& stage_id) const override;
  double GetBackwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                               const int32_t& stage_id) const override;
};
using PReLUCostPtr = std::shared_ptr<PReLUCost>;

class OneHotCost : public OperatorCost {
 public:
  OneHotCost() = default;
  ~OneHotCost() override = default;

  // per device communication cost
  double GetCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                     const int32_t& stage_id) const override {
    return GetForwardCommCost(inputs, outputs, stage_id) + GetBackwardCommCost(inputs, outputs, stage_id);
  }
  double GetForwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                            const int32_t& stage_id) const override;
  double GetBackwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                             const int32_t& stage_id) const override;

  // per device computation cost
  double GetMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                       const int32_t& stage_id) const override {
    return GetForwardMemoryCost(inputs, outputs, stage_id) + GetBackwardMemoryCost(inputs, outputs, stage_id);
  }
  double GetForwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                              const int32_t& stage_id) const override;
  double GetBackwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                               const int32_t& stage_id) const override;
};
using OneHotCostPtr = std::shared_ptr<OneHotCost>;

class SoftmaxCrossEntropyWithLogitsCost : public OperatorCost {
 public:
  SoftmaxCrossEntropyWithLogitsCost() = default;
  ~SoftmaxCrossEntropyWithLogitsCost() override = default;

  // per device communication cost
  double GetCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                     const int32_t& stage_id) const override {
    return GetForwardCommCost(inputs, outputs, stage_id) + GetBackwardCommCost(inputs, outputs, stage_id);
  }
  double GetForwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                            const int32_t& stage_id) const override;
  double GetBackwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                             const int32_t& stage_id) const override;

  // per device computation cost
  double GetMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                       const int32_t& stage_id) const override {
    return GetForwardMemoryCost(inputs, outputs, stage_id) + GetBackwardMemoryCost(inputs, outputs, stage_id);
  }
  double GetForwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                              const int32_t& stage_id) const override;
  double GetBackwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                               const int32_t& stage_id) const override;
};
using SoftmaxCrossEntropyWithLogitsCostPtr = std::shared_ptr<SoftmaxCrossEntropyWithLogitsCost>;

class ReshapeCost : public OperatorCost {
 public:
  ReshapeCost() = default;

  ~ReshapeCost() override = default;

  // per device communication cost
  double GetCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                     const int32_t& stage_id) const override {
    return GetForwardCommCost(inputs, outputs, stage_id) + GetBackwardCommCost(inputs, outputs, stage_id);
  }

  double GetForwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                            const int32_t& stage_id) const override;

  double GetBackwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                             const int32_t& stage_id) const override;

  // per device computation cost
  double GetMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                       const int32_t& stage_id) const override {
    return GetForwardMemoryCost(inputs, outputs, stage_id) + GetBackwardMemoryCost(inputs, outputs, stage_id);
  }

  double GetForwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                              const int32_t& stage_id) const override;

  double GetBackwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                               const int32_t& stage_id) const override;
};
using ReshapeCostPtr = std::shared_ptr<ReshapeCost>;

class ArithmeticCost : public OperatorCost {
 public:
  ArithmeticCost() = default;
  ~ArithmeticCost() override = default;

  double GetCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                     const int32_t& stage_id) const override {
    return GetForwardCommCost(inputs, outputs, stage_id) + GetBackwardCommCost(inputs, outputs, stage_id);
  }
  double GetForwardCommCost(const std::vector<TensorInfo>&, const std::vector<TensorInfo>&,
                            const int32_t&) const override {
    return 0.0;
  }
  double GetBackwardCommCost(const std::vector<TensorInfo>&, const std::vector<TensorInfo>&,
                             const int32_t&) const override;

  double GetMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                       const int32_t& stage_id) const override {
    return GetForwardMemoryCost(inputs, outputs, stage_id) + GetBackwardMemoryCost(inputs, outputs, stage_id);
  }
  double GetForwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                              const int32_t& stage_id) const override;
  double GetBackwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                               const int32_t& stage_id) const override;
};
using ArithmeticCostPtr = std::shared_ptr<ArithmeticCost>;

class L2NormalizeCost : public OperatorCost {
 public:
  L2NormalizeCost() = default;
  ~L2NormalizeCost() override = default;

  double GetCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                     const int32_t& stage_id) const override {
    return GetForwardCommCost(inputs, outputs, stage_id) + GetBackwardCommCost(inputs, outputs, stage_id);
  }
  double GetForwardCommCost(const std::vector<TensorInfo>&, const std::vector<TensorInfo>&,
                            const int32_t&) const override {
    return 0.0;
  }
  double GetBackwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                             const int32_t& stage_id) const override;
  double GetMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                       const int32_t& stage_id) const override {
    return GetForwardMemoryCost(inputs, outputs, stage_id) + GetBackwardMemoryCost(inputs, outputs, stage_id);
  }
  double GetForwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                              const int32_t& stage_id) const override;
  double GetBackwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                               const int32_t& stage_id) const override;
};
using L2NormalizeCostPtr = std::shared_ptr<L2NormalizeCost>;

class ReduceMethodCost : public OperatorCost {
 public:
  ReduceMethodCost() = default;
  ~ReduceMethodCost() override = default;

  double GetCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                     const int32_t& stage_id) const override {
    return GetForwardCommCost(inputs, outputs, stage_id) + GetBackwardCommCost(inputs, outputs, stage_id);
  }
  double GetForwardCommCost(const std::vector<TensorInfo>&, const std::vector<TensorInfo>&,
                            const int32_t& stage_id) const override;
  double GetBackwardCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                             const int32_t& stage_id) const override;
  double GetMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                       const int32_t& stage_id) const override {
    return GetForwardMemoryCost(inputs, outputs, stage_id) + GetBackwardCommCost(inputs, outputs, stage_id);
  }
  double GetForwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                              const int32_t& stage_id) const override;
  double GetBackwardMemoryCost(const std::vector<TensorInfo>&, const std::vector<TensorInfo>&,
                               const int32_t&) const override {
    return 0.0;
  }
  void set_cross_batch(bool cb) { cross_batch_ = cb; }

 protected:
  bool cross_batch_ = false;
};
using ReduceMethodCostPtr = std::shared_ptr<ReduceMethodCost>;

class ReduceMeanCost : public ReduceMethodCost {
 public:
  ReduceMeanCost() = default;
  ~ReduceMeanCost() override = default;

  double GetForwardMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                              const int32_t& stage_id) const override;
};
using ReduceMeanCostPtr = std::shared_ptr<ReduceMeanCost>;

class GetNextCost : public OperatorCost {
 public:
  GetNextCost() = default;
  ~GetNextCost() override = default;

  double GetCommCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                     const int32_t& stage_id) const override {
    return GetForwardCommCost(inputs, outputs, stage_id) + GetBackwardCommCost(inputs, outputs, stage_id);
  }
  double GetForwardCommCost(const std::vector<TensorInfo>&, const std::vector<TensorInfo>&,
                            const int32_t&) const override {
    return 0.0;
  }
  double GetBackwardCommCost(const std::vector<TensorInfo>&, const std::vector<TensorInfo>&,
                             const int32_t&) const override {
    return 0.0;
  }
  double GetMemoryCost(const std::vector<TensorInfo>& inputs, const std::vector<TensorInfo>& outputs,
                       const int32_t& stage_id) const override {
    return GetForwardMemoryCost(inputs, outputs, stage_id) + GetBackwardMemoryCost(inputs, outputs, stage_id);
  }
  // Inputs vector is empty for generator ops.
  double GetForwardMemoryCost(const std::vector<TensorInfo>&, const std::vector<TensorInfo>&,
                              const int32_t&) const override {
    return 0.0;
  }
  // Generator ops don't have backward steps.
  double GetBackwardMemoryCost(const std::vector<TensorInfo>&, const std::vector<TensorInfo>&,
                               const int32_t&) const override {
    return 0.0;
  }
};
using GetNextCostPtr = std::shared_ptr<GetNextCost>;
}  // namespace parallel
}  // namespace mindspore
#endif  // PARALLEL_AUTO_PARALLEL_OPERATOR_COSTMODEL_H_
