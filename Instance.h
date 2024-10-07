#ifndef VULKANINSTANCE_H
#define VULKANINSTANCE_H

#include <string_view>
#include <span>
#include "vulkan/vulkan.hpp"

namespace VKKit {
class Window;

// A Vulkan instance. Wrapper over VkInstance.
class Instance {
public:
    Instance() noexcept;
    
    /**
     * @brief Construct a Vulkan instance
     * 
     * @param app_name The name of the application
     * 
     * @throw std::runtime_error with error information on failure
     */
    Instance(std::string_view app_name, const Window& window);

#ifndef NDEBUG
    /**
     * @brief Construct a Vulkan instance with debugging features
     * 
     * @param app_name The name of the application
     * @param validation_layers The validation layers that will be enabled for the debugging
     * 
     * @throw std::runtime_error with error information on failure
     */
    Instance(std::string_view app_name, std::span<const char* const> validation_layers, const Window& window);
#endif
    ~Instance();

    Instance(const Instance& i) = delete;
    Instance& operator=(const Instance& i) = delete;
    Instance(Instance&& i) noexcept;
    Instance& operator=(Instance&& i) noexcept;

    VkInstance Get() const noexcept { return instance; }

private:
    VkInstance instance;
};
}

#endif