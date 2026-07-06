#pragma once

#include "mex.hpp"
#include "MatlabDataArray.hpp"

#include <functional>
#include <string>
#include <unordered_map>

namespace ps_mex {

using CommandFunc = std::function<void(matlab::mex::ArgumentList& outputs,
                                       matlab::mex::ArgumentList& inputs,
                                       matlab::engine::MATLABEngine* matlabPtr)>;

class CommandRegistry {
public:
  static CommandRegistry& instance();

  void registerCommand(const std::string& name, CommandFunc func);
  CommandFunc find(const std::string& name) const;

private:
  CommandRegistry() = default;
  std::unordered_map<std::string, CommandFunc> commands_;
};

} // namespace ps_mex
