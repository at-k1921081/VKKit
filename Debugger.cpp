#include <iostream>
#include "Debugger.h"
#include "VkResultString.h"

namespace VKKit {
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user_data)
{
    std::cerr << "Validation layer: " << data->pMessage << '\n';

    return VK_FALSE;
}

Debugger::Debugger() : instance{ nullptr }, destructor{ nullptr }, debugger{ nullptr }
{}

Debugger::Debugger(VkInstance instance, VkDebugUtilsMessageSeverityFlagsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type) :
    instance{ instance }
{
    const VkDebugUtilsMessengerCreateInfoEXT create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = severity,
        .messageType = type,
        .pfnUserCallback = DebugCallback
    };

    const auto constructor = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,
        "vkCreateDebugUtilsMessengerEXT"));
    if (!constructor) throw std::runtime_error("Failed to load function vkCreateDebugUtilsMessengerEXT");

    destructor = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,
        "vkDestroyDebugUtilsMessengerEXT"));
    if (!destructor) throw std::runtime_error("Failed to load function vkCreateDebugUtilsMessengerEXT");

    const auto result = constructor(instance, &create_info, nullptr, &debugger);
    if (result != VK_SUCCESS) ThrowError("Failed to create debug messenger.", result);
}

Debugger::~Debugger()
{
    if (instance && destructor) destructor(instance, debugger, nullptr);
}

Debugger::Debugger(Debugger&& d) noexcept : instance{ d.instance }, destructor{ d.destructor }, debugger{ d.debugger }
{
    d.instance = nullptr;
    d.destructor = nullptr;
    d.debugger = nullptr;
}

Debugger& Debugger::operator=(Debugger&& d) noexcept
{
    if (instance && destructor) destructor(instance, debugger, nullptr);
    instance = d.instance;
    d.instance = nullptr;
    destructor = d.destructor;
    d.destructor = nullptr;
    debugger = d.debugger;
    d.debugger = nullptr;
    return *this;
}
}