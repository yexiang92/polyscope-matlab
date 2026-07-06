#include "command_router.h"

namespace ps_mex {

CommandRegistry& CommandRegistry::instance() {
  static CommandRegistry reg;
  return reg;
}

void CommandRegistry::registerCommand(const std::string& name, CommandFunc func) {
  commands_[name] = func;
}

CommandFunc CommandRegistry::find(const std::string& name) const {
  auto it = commands_.find(name);
  if (it == commands_.end()) {
    return nullptr;
  }
  return it->second;
}

} // namespace ps_mex
