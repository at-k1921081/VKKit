#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "vulkan/vulkan.h"

namespace VKKit {
class Debugger {
public:
    Debugger();
    Debugger(VkInstance instance,
        VkDebugUtilsMessageSeverityFlagsEXT severity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        VkDebugUtilsMessageTypeFlagsEXT type =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT);
    ~Debugger();

    Debugger(const Debugger& d) = delete;
    Debugger& operator=(const Debugger& d) = delete;
    Debugger(Debugger&& d) noexcept;
    Debugger& operator=(Debugger&& d) noexcept;

    VkDebugUtilsMessengerEXT GetDebugger() const { return debugger; }

private:
    VkInstance instance;
    PFN_vkDestroyDebugUtilsMessengerEXT destructor;
    VkDebugUtilsMessengerEXT debugger;
};
}

#endif