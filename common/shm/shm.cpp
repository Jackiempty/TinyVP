// common/shm/shm.cpp
#include "shm.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>

// ==========================================
// 1. Lifecycle Management (Constructor & Destructor)
// ==========================================

SharedMemorySegment::SharedMemorySegment(const std::string& shm_name, bool clear_on_init) : name(shm_name) {
  // 1. Open or create POSIX shared memory
  fd = shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
  if (fd == -1) { throw std::runtime_error("Failed to shm_open: " + name); }

  // 2. Set the size of the shared memory to the exact size of ShmLayout
  if (ftruncate(fd, sizeof(ShmLayout)) == -1) { throw std::runtime_error("Failed to ftruncate SHM"); }

  // 3. Map the memory into the current Process space and cast it to a ShmLayout pointer
  void* ptr = mmap(nullptr, sizeof(ShmLayout), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) { throw std::runtime_error("Failed to mmap SHM"); }
  layout = static_cast<ShmLayout*>(ptr);

  // 4. Initialization clearance when the Server (HAL) starts
  if (clear_on_init) {
    std::memset(layout, 0, sizeof(ShmLayout));

    // Ensure atomic variables are correctly zeroed out
    layout->head.store(0, std::memory_order_relaxed);
    layout->tail.store(0, std::memory_order_relaxed);
  }
}

SharedMemorySegment::~SharedMemorySegment() {
  // Unmap memory and close the file descriptor
  if (layout) { munmap(layout, sizeof(ShmLayout)); }
  if (fd != -1) { close(fd); }
}

// ==========================================
// 2. Host-side API (Push)
// ==========================================

uint32_t SharedMemorySegment::push_command(const Command& cmd) {
  // Get the current read/write pointers
  uint32_t current_head = layout->head.load(std::memory_order_acquire);
  uint32_t current_tail = layout->tail.load(std::memory_order_acquire);

  uint32_t next_head = (current_head + 1) % CMD_QUEUE_SIZE;

  // Check if the Queue is full
  while (next_head == current_tail) {
    // Queue is full, yield slightly to wait for HAL to digest commands
    std::this_thread::yield();
    current_tail = layout->tail.load(std::memory_order_acquire);  // Read tail again
  }

  // Write the command to the current_head position of the Queue
  layout->cmd_queue[current_head] = cmd;

  // Update the head pointer
  layout->head.store(next_head, std::memory_order_release);

  return current_head;  // Return this ticket (Index) so the Host can track it
}

void SharedMemorySegment::wait_for_command_done(uint32_t cmd_index) {
  // Host side polls the status of a specific command
  while (layout->cmd_queue[cmd_index].status != CmdStatus::DONE) {
    // Short sleep to avoid 100% CPU usage
    std::this_thread::sleep_for(std::chrono::microseconds(1));
  }
}

// ==========================================
// 3. HAL-side API (Fetch)
// ==========================================

bool SharedMemorySegment::has_new_command() const {
  // As long as head is not equal to tail, it means there are new commands waiting to be processed
  uint32_t current_head = layout->head.load(std::memory_order_acquire);
  uint32_t current_tail = layout->tail.load(std::memory_order_acquire);
  return current_head != current_tail;
}

Command* SharedMemorySegment::fetch_command() {
  uint32_t current_head = layout->head.load(std::memory_order_acquire);
  uint32_t current_tail = layout->tail.load(std::memory_order_acquire);

  // Failsafe: return nullptr if there are actually no new commands
  if (current_head == current_tail) { return nullptr; }

  // Get the pointer to the command to be processed
  Command* cmd = &layout->cmd_queue[current_tail];

  // Update the tail pointer
  uint32_t next_tail = (current_tail + 1) % CMD_QUEUE_SIZE;
  layout->tail.store(next_tail, std::memory_order_release);

  return cmd;
}
