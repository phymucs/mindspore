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

#include "dataset/kernels/image/random_vertical_flip_op.h"

#include "dataset/kernels/image/image_utils.h"
#include "dataset/util/status.h"

namespace mindspore {
namespace dataset {
const float RandomVerticalFlipOp::kDefProbability = 0.5;

Status RandomVerticalFlipOp::Compute(const std::shared_ptr<Tensor> &input, std::shared_ptr<Tensor> *output) {
  IO_CHECK(input, output);
  if (distribution_(rnd_)) {
    return VerticalFlip(input, output);
  }
  *output = input;
  return Status::OK();
}
}  // namespace dataset
}  // namespace mindspore
