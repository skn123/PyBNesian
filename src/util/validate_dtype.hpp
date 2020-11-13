#ifndef PYBNESIAN_VALIDATE_DTYPE_HPP
#define PYBNESIAN_VALIDATE_DTYPE_HPP

#include <pybind11/pybind11.h>
#include <dataset/dataset.hpp>
#include <factors/factors.hpp>
#include <util/util_types.hpp>

namespace py = pybind11;

using namespace dataset;
using factors::FactorType;
using util::ArcVector, util::FactorTypeVector;

namespace util {
    
    void check_edge_list(const DataFrame& df, const ArcVector& list);
    void check_node_type_list(const DataFrame& df, const FactorTypeVector& list);

    std::shared_ptr<arrow::DataType> to_type(arrow::Type::type t); 


}

#endif //PYBNESIAN_VALIDATE_DTYPE_HPP