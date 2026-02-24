// common/shm/shm.hpp
#pragma once

#include <atomic>
#include <cstdint>
#include <string>

#include "types.hpp"

constexpr uint32_t CMD_QUEUE_SIZE   = 64;
constexpr uint32_t PAYLOAD_CAPACITY = 8192;

struct ShmLayout {
  std::atomic<uint32_t> head;
  std::atomic<uint32_t> tail;

  Command cmd_queue[CMD_QUEUE_SIZE];
  int32_t payload[PAYLOAD_CAPACITY];
};

class SharedMemorySegment {
  public:
  std::string name;
  int         fd;
  ShmLayout*  layout;

  explicit SharedMemorySegment(const std::string& shm_name, bool clear_on_init = false);
  ~SharedMemorySegment();

  // --- Host API ---
  uint32_t push_command(const Command& cmd);
  void     wait_for_command_done(uint32_t cmd_index);

  // --- HAL API ---
  bool     has_new_command() const;
  Command* fetch_command();
};
