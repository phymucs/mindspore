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
#include <iostream>
#include <utility>

#include "dataset/engine/execution_tree.h"
#include "dataset/engine/datasetops/repeat_op.h"
#include "dataset/engine/data_buffer.h"
#include "dataset/engine/db_connector.h"

#include "utils/log_adapter.h"

namespace mindspore {
namespace dataset {
// Builder constructor.  Creates the builder object.
RepeatOp::Builder::Builder(int32_t count) : build_max_repeats_(count) {}

Status RepeatOp::Builder::SanityCheck() const {
  if (build_max_repeats_ < kInfiniteRepeat || build_max_repeats_ == 0) {
    std::string err_msg("Repeat count must be > 0 or -1.");
    RETURN_STATUS_UNEXPECTED(err_msg);
  }
  return Status::OK();
}

// The builder "build" method creates the final object.
Status RepeatOp::Builder::Build(std::shared_ptr<RepeatOp> *ptr) {
  RETURN_IF_NOT_OK(SanityCheck());
  *ptr = std::make_shared<RepeatOp>(build_max_repeats_);
  return Status::OK();
}

// Constructor of the RepeatOp.
RepeatOp::RepeatOp(int32_t count) : PipelineOp(0), max_repeats_(count), repeat_count_(0) {}

// Destructor
RepeatOp::~RepeatOp() {}

// A print method typically used for debugging
void RepeatOp::Print(std::ostream &out, bool show_all) const {
  // Call base class printer first
  PipelineOp::Print(out, show_all);

  // Then display our own stuff
  out << "RepeatOp:"
      << "\nCurrent repeat count: " << repeat_count_ << "\nMax repeat count: " << max_repeats_
      << "\nLeaf Nodes in my execution path:";
  if (!leaf_ops_.empty()) {
    out << "\n";
    for (size_t i = 0; i < leaf_ops_.size(); i++) {
      out << "  Operator: " << leaf_ops_[i]->id() << "\n";
    }
  } else {
    out << " kNone.";
  }
  out << "\n-------------------------\n\n";  // End the display with this line
}

// Base-class override for executing specific RepeatOp configurations. This code will be called
// during the execution tree prepare phase when it is visiting this operator.
Status RepeatOp::PrepareNodeAction() {
  // Run any common code from super class first before adding our own specific logic
  RETURN_IF_NOT_OK(PipelineOp::PrepareNodeAction());
  std::shared_ptr<DatasetOp> leaf_op = tree_->PopFromRepeatStack();
  while (leaf_op != nullptr) {
    // Track the leaf operators that are under this repeat op.
    leaf_ops_.push_back(leaf_op);

    // Special case.  If the repeat count is 1, then pre-flag the leaf nodes
    // to tell them they are already at their last op:
    if (max_repeats_ == 1) {
      leaf_op->set_control_flag(kDeOpLastRepeat);
    }
    leaf_op = tree_->PopFromRepeatStack();
  }
  return Status::OK();
}

// Base-class override for setting specific RepeatOp configurations. This code will be called
// during the execution tree prepare phase BEFORE traversing down to child operators.
uint32_t RepeatOp::PrepareFlags() const { return ExecutionTree::kDePrepRepeat; }

// This function returns the buffer that is at the top of our output connector. The caller is
// typically our parent node, when the parent is asking us to provide the next buffer of data.
// Since RepeatOp is an inlined op, getting a buffer from us will simply bounce you to get
// a buffer from our child.
// This function sets the `retryIfEoe` flag when popping from the child connector. This way,
// this function will retry to pop the connector again and will get the non-EOE buffer if any.
Status RepeatOp::GetNextBuffer(std::unique_ptr<DataBuffer> *p_buffer, int32_t worker_id, bool retry_if_eoe) {
  if (child_.empty()) {
    RETURN_STATUS_UNEXPECTED("RepeatOp can't be the leaf node.");
  }

  std::unique_ptr<DataBuffer> buf;
  RETURN_IF_NOT_OK(child_[0]->GetNextBuffer(&buf, worker_id, true));
  // Loop until non EOE is received
  while (buf->eoe()) {
    RETURN_IF_NOT_OK(EoeReceived(worker_id));
    if (state_ == OpState::kDeOpIdle) {
      *p_buffer = std::move(buf);
      return Status::OK();
    }
    RETURN_IF_NOT_OK(child_[0]->GetNextBuffer(&buf, worker_id, true));
  }
  // Check if the last buf is next eof
  if (buf->eof()) {
    RETURN_IF_NOT_OK(EofReceived(worker_id));
  }
  *p_buffer = std::move(buf);
  return Status::OK();
}

// Base-class override for handling cases when an eoe is received.
Status RepeatOp::EoeReceived(int32_t worker_id) {
  repeat_count_++;
  MS_LOG(INFO) << "Repeat operator end of epoch message received. Repeat count is now: " << repeat_count_ << ".";

  // If we've reached the requested repeat count, then flag the leaf nodes
  // to tell them they've got one more epoch to perform.  When they reach the end
  // of the last epoch, they quit rather than loop again.
  if (max_repeats_ != kInfiniteRepeat && repeat_count_ == (max_repeats_ - 1)) {
    for (size_t i = 0; i < leaf_ops_.size(); i++) {
      leaf_ops_[i]->set_control_flag(kDeOpLastRepeat);
    }
  }
  if (repeat_count_ == max_repeats_) {
    state_ = OpState::kDeOpIdle;
    return Status::OK();
  }

  //  base-class ResetSubtree
  return (DatasetOp::ResetSubtree());
}

// Class functor operator () override.
// Most dataset ops operate by launching a thread (see ExecutionTree).
// However, the RepeatOp is defined as a inlined operator, so it is invalid to launch the
// functor since this op runs inlined inside another operator.  The function is overloaded to
// ensure that it is not called by mistake (it will generate an error).
Status RepeatOp::operator()() { RETURN_STATUS_UNEXPECTED("Logic error. RepeatOp is an inlined operator."); }

// Base-class override for handling cases when an eof is received.
Status RepeatOp::EofReceived(int32_t worker_id) {
  MS_LOG(INFO) << "Repeat operator EOF received, do nothing now.";
  return Status::OK();
}

int32_t RepeatOp::num_consumers() const {
  if (parent_.empty()) {
    MS_LOG(INFO) << "Repeat operator, no parent node, assuming it's root and returning 1.";
    return 1;
  } else if (parent_[0] == nullptr) {
    MS_LOG(INFO) << "Repeat operator, pointer to the first parent is null. Returning 0.";
    return 0;
  } else {
    return parent_[0]->num_consumers();
  }
}

int32_t RepeatOp::num_producers() const {
  if (child_.empty() || child_[0] == nullptr) {
    MS_LOG(INFO) << "Repeat operator, pointer to child node is null. Returning 0.";
    return 0;
  } else {
    return child_[0]->num_producers();
  }
}
}  // namespace dataset
}  // namespace mindspore
