// hal/hal.cpp
#include "hal.hpp"

#include <cstdlib>

#include "hal/ipc/ipc.hpp"

namespace hal {

std::shared_ptr<IDevice> get_device() {
  static std::shared_ptr<IDevice> instance = nullptr;
  if (!instance) {
    // Decide which Backend to be loaded according to Env Var
    const char* mode = std::getenv("TINYVP_BACKEND");
    if (mode && std::string(mode) == "DRIVER") {
      // future： instance = std::make_shared<driver::DriverDevice>("/dev/tinyvp");
      throw std::runtime_error("Driver backend not implemented yet!");
    } else {
      // default: IPC
      instance = std::make_shared<ipc::IpcDevice>();
    }
  }
  return instance;
}

}  // namespace hal
