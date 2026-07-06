#pragma once

#include "mex.hpp"
#include "MatlabDataArray.hpp"

namespace ps_mex {

// Indexing helpers for matlab::mex::ArgumentList.
//
// In R2023a ArgumentList is a matlab::mex::MexIORange, whose full definition
// (including operator[]) is only available in the single TU that includes
// mexAdapter.hpp. These helpers are defined in mex_io_helpers.cpp (that TU),
// so the rest of the binding code can access inputs/outputs without needing
// the full MexIORange definition in every translation unit.

const matlab::data::Array& getInput(matlab::mex::ArgumentList& inputs, size_t idx);
matlab::data::Array& getOutput(matlab::mex::ArgumentList& outputs, size_t idx);
size_t inputCount(matlab::mex::ArgumentList& inputs);

} // namespace ps_mex
