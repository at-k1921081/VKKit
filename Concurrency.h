#ifndef CONCURRENCY_H
#define CONCURRENCY_H

#include <numeric>
#include "vulkan/vulkan.hpp"

namespace VKKit {
class Semaphore {
public:
    Semaphore();
    Semaphore(VkDevice device);
    ~Semaphore();

    Semaphore(const Semaphore& s) = delete;
    Semaphore& operator=(const Semaphore& s) = delete;
    Semaphore(Semaphore&& s) noexcept;
    Semaphore& operator=(Semaphore&& s) noexcept;

    VkSemaphore Get() const { return semaphore; }

    void Wait(uint64_t timeout = std::numeric_limits<uint64_t>::max()) const;

private:
    VkSemaphore semaphore;
    VkDevice device;
};

class Fence {
public:
    Fence();
    Fence(VkDevice device, bool signaled = true);
    ~Fence();

    Fence(const Fence& f) = delete;
    Fence& operator=(const Fence& f) = delete;
    Fence(Fence&& f) noexcept;
    Fence& operator=(Fence&& f) noexcept;

    VkFence Get() const { return fence; }

    void Wait(uint64_t timeout = std::numeric_limits<uint64_t>::max()) const;
    void Reset() const;

private:
    VkFence fence;
    VkDevice device;
};
}

#endif