// Copyright (C) 2018-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "infer_request.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "itt.hpp"
#include "openvino/core/except.hpp"
#include "openvino/runtime/profiling_info.hpp"
#include "openvino/runtime/tensor.hpp"
#include "plugin.hpp"

using Time = std::chrono::high_resolution_clock;

namespace {

void allocate_tensor_impl(ov::Tensor& tensor, const ov::element::Type& element_type, const ov::Shape& shape) {
    if (!tensor || tensor.get_element_type() != element_type) {
        tensor = ov::Tensor(element_type, shape);
    } else {
        tensor.set_shape(shape);
    }
}

}  // namespace

ov::auto_plugin::InferRequest::InferRequest(const std::shared_ptr<const ov::auto_plugin::CompiledModel>& model,
                                            const SoAsyncInferRequest& request_to_share_tensors_with)
    : ov::ISyncInferRequest(model),
      m_shared_request(request_to_share_tensors_with) {
    if (!m_shared_request) {
        // Allocate input/output tensors
        for (const auto& input : get_inputs()) {
            allocate_tensor(input, [input](ov::Tensor& tensor) {
                // Can add a check to avoid double work in case of shared tensors
                allocate_tensor_impl(tensor,
                                    input.get_element_type(),
                                    input.get_partial_shape().is_dynamic() ? ov::Shape{0} : input.get_shape());
            });
        }
        for (const auto& output : get_outputs()) {
            allocate_tensor(output, [output](ov::Tensor& tensor) {
                // Can add a check to avoid double work in case of shared tensors
                allocate_tensor_impl(tensor,
                                    output.get_element_type(),
                                    output.get_partial_shape().is_dynamic() ? ov::Shape{0} : output.get_shape());
            });
        }
    } else {
        for (const auto& input : get_inputs()) {
            ov::ISyncInferRequest::set_tensor(input, ov::Tensor(m_shared_request->get_tensor(input), m_shared_request._so));
        }
        for (const auto& output : get_outputs()) {
            ov::ISyncInferRequest::set_tensor(output, ov::Tensor(m_shared_request->get_tensor(output), m_shared_request._so));
        }
    }
}


const ov::auto_plugin::SoAsyncInferRequest& ov::auto_plugin::InferRequest::get_shared_request() {
    return m_shared_request;
}

void ov::auto_plugin::InferRequest::set_scheduled_request(SoAsyncInferRequest request) {
    m_scheduled_request = request;
}

void ov::auto_plugin::InferRequest::set_tensors_to_another_request(const SoAsyncInferRequest& req) {
    for (const auto &it : get_inputs()) {
        // this request is already in BUSY state, so using the internal functions safely
        auto tensor = get_tensor(it);
        auto type = tensor.get_element_type();
        bool is_remote  = tensor.is<ov::RemoteTensor>() || req->get_tensor(it).is<ov::RemoteTensor>();
        if (is_remote || req->get_tensor(it).data(type) != tensor.data(type))
            req->set_tensor(it, tensor);
    }
    for (const auto &it : get_outputs()) {
        // this request is already in BUSY state, so using the internal functions safely
        auto tensor = get_tensor(it);
        auto type = tensor.get_element_type();
        bool is_remote  = tensor.is<ov::RemoteTensor>() || req->get_tensor(it).is<ov::RemoteTensor>();
        if (is_remote || req->get_tensor(it).data(type) != tensor.data(type))
            req->set_tensor(it, tensor);
    }
}

void ov::auto_plugin::InferRequest::set_tensor(const ov::Output<const ov::Node>& port, const ov::Tensor& tensor) {
    if (m_shared_request)
        m_shared_request->set_tensor(port, tensor);
    ov::ISyncInferRequest::set_tensor(port, tensor);
}


void ov::auto_plugin::InferRequest::infer() {
    OPENVINO_NOT_IMPLEMENTED;
}

std::vector<ov::ProfilingInfo> ov::auto_plugin::InferRequest::get_profiling_info() const {
    if (m_shared_request)
        return m_shared_request->get_profiling_info();
    if (m_scheduled_request)
        return m_scheduled_request->get_profiling_info();
    OPENVINO_NOT_IMPLEMENTED;
}

ov::auto_plugin::InferRequest::~InferRequest() = default;

std::vector<std::shared_ptr<ov::IVariableState>> ov::auto_plugin::InferRequest::query_state() const {
    if (m_shared_request)
        return m_shared_request->query_state();
    OPENVINO_NOT_IMPLEMENTED;
}
