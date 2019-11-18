#include "absl/memory/memory.h"
#include "fast_transformers/core/blas.h"
#include "fast_transformers/core/profiler.h"
#include "fast_transformers/core/tensor.h"
#include "fast_transformers/layers/bert_attention.h"
#include "fast_transformers/layers/bert_embedding.h"
#include "fast_transformers/layers/bert_intermediate.h"
#include "fast_transformers/layers/bert_output.h"
#include "fast_transformers/layers/bert_self_attention.h"
#include "pybind11/pybind11.h"

namespace fast_transformers {
namespace python {

namespace py = pybind11;

static void DLPack_Capsule_Destructor(PyObject *data) {
  auto *dlMTensor = (DLManagedTensor *)PyCapsule_GetPointer(data, "dltensor");
  if (dlMTensor) {
    // the dlMTensor has not been consumed, call deleter ourselves
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    dlMTensor->deleter(const_cast<DLManagedTensor *>(dlMTensor));
  } else {
    // the dlMTensor has been consumed
    // PyCapsule_GetPointer has set an error indicator
    PyErr_Clear();
  }
}

PYBIND11_MODULE(fast_transformers, m) {
  m.def("auto_init_blas", &core::AutoInitBlas);
  m.def("enable_gperf", &core::EnableGperf);
  m.def("disable_gperf", &core::DisableGperf);

  py::class_<core::Tensor>(m, "Tensor")
      .def_static("from_dlpack",
                  [](py::capsule capsule) -> std::unique_ptr<core::Tensor> {
                    auto tensor = (DLManagedTensor *)(capsule);
                    PyCapsule_SetName(capsule.ptr(), "used_tensor");
                    return absl::make_unique<core::Tensor>(tensor);
                  })
      .def("to_dlpack",
           [](core::Tensor &tensor) -> py::capsule {
             auto *dlpack = tensor.ToDLPack();
             return py::capsule(dlpack, "dltensor", DLPack_Capsule_Destructor);
           })
      .def("n_dim", &core::Tensor::n_dim)
      .def("shape", &core::Tensor::shape)
      .def("float_data", &core::Tensor::data<float>);

  py::class_<layers::BERTEmbedding>(m, "BERTEmbedding")
      .def(py::init(
          [](core::Tensor &word_embeddings, core::Tensor &position_embeddings,
             core::Tensor &token_type_embeddings,
             core::Tensor &layer_norm_weights, core::Tensor &layer_norm_bias,
             float dropout_rate) -> layers::BERTEmbedding * {
            return new layers::BERTEmbedding(
                std::move(word_embeddings), std::move(position_embeddings),
                std::move(token_type_embeddings), std::move(layer_norm_weights),
                std::move(layer_norm_bias), dropout_rate);
          }))
      .def("__call__",
           [](layers::BERTEmbedding &self, core::Tensor &input_ids,
              core::Tensor &token_type_ids, core::Tensor &position_ids) {
             return self(std::move(input_ids), std::move(token_type_ids),
                         std::move(position_ids));
           });

  py::class_<layers::BertAttention>(m, "BertAttention")
      .def(py::init([](core::Tensor &query_weight, core::Tensor &query_bias,
                       core::Tensor &key_weight, core::Tensor &key_bias,
                       core::Tensor &value_weight, core::Tensor &value_bias,
                       core::Tensor &dense_weight, core::Tensor &dense_bias,
                       core::Tensor &layer_norm_weight,
                       core::Tensor &layer_norm_bias,
                       int num_attention_heads) -> layers::BertAttention * {
        return new layers::BertAttention(
            std::move(query_weight), std::move(query_bias),
            std::move(key_weight), std::move(key_bias), std::move(value_weight),
            std::move(value_bias), std::move(dense_weight),
            std::move(dense_bias), std::move(layer_norm_weight),
            std::move(layer_norm_bias), num_attention_heads);
      }))
      .def("__call__",
           [](layers::BertAttention &self, core::Tensor &input_tensor,
              core::Tensor &attention_mask, core::Tensor &head_mask) {
             return self(std::move(input_tensor), std::move(attention_mask),
                         std::move(head_mask));
           });

  py::class_<layers::BertSelfAttention>(m, "BertSelfAttention")
      .def(py::init([](core::Tensor &qkv_weight, core::Tensor &qkv_bias,
                       core::Tensor &dense_weight, core::Tensor &dense_bias,
                       core::Tensor &layer_norm_weight,
                       core::Tensor &layer_norm_bias,
                       int num_attention_heads) -> layers::BertSelfAttention * {
        return new layers::BertSelfAttention(
            std::move(qkv_weight), std::move(qkv_bias), std::move(dense_weight),
            std::move(dense_bias), std::move(layer_norm_weight),
            std::move(layer_norm_bias), num_attention_heads);
      }))
      .def("__call__",
           [](layers::BertSelfAttention &self, core::Tensor &input_tensor,
              core::Tensor &attention_mask, core::Tensor &head_mask) {
             return self(std::move(input_tensor), std::move(attention_mask),
                         std::move(head_mask));
           });

  py::class_<layers::BertIntermediate>(m, "BertIntermediate")
      .def(py::init([](core::Tensor &dense_weight,
                       core::Tensor &dense_bias) -> layers::BertIntermediate * {
        return new layers::BertIntermediate(std::move(dense_weight),
                                            std::move(dense_bias));
      }))
      .def("__call__",
           [](layers::BertIntermediate &self, core::Tensor &input_tensor) {
             return self(std::move(input_tensor));
           });

  py::class_<layers::BertOutput>(m, "BertOutput")
      .def(py::init([](core::Tensor &dense_weight, core::Tensor &dense_bias,
                       core::Tensor &layer_norm_weight,
                       core::Tensor &layer_norm_bias) -> layers::BertOutput * {
        return new layers::BertOutput(
            std::move(dense_weight), std::move(dense_bias),
            std::move(layer_norm_weight), std::move(layer_norm_bias));
      }))
      .def("__call__", [](layers::BertOutput &self, core::Tensor &hidden_states,
                          core::Tensor &input_tensor) {
        return self(std::move(hidden_states), std::move(input_tensor));
      });
}

}  // namespace python
}  // namespace fast_transformers