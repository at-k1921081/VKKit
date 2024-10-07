#include "SDL_vulkan.h"
#include "Instance.h"
#include "VkResultString.h"
#include "Windowing.h"

namespace VKKit {
#ifndef NDEBUG
static bool CheckValidationLayerSupport(std::span<const char* const> validation_layers)
{
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for (auto name : validation_layers) {
        if (std::find_if(available_layers.begin(), available_layers.end(), [name](VkLayerProperties prop){
            return std::char_traits<char>::compare(name, prop.layerName, std::min(std::char_traits<char>::length(name),
                std::char_traits<char>::length(prop.layerName)));
        }) == available_layers.end())
            return false;
    }

    return true;
}
#endif

static std::vector<const char*> GetRequiredExtensions(const Window& window)
{
    uint32_t count = 0;
    SDL_Vulkan_GetInstanceExtensions(window.Get(), &count, nullptr);

    std::vector<const char*> extensions(count);

    const auto result = SDL_Vulkan_GetInstanceExtensions(window.Get(), &count, extensions.data());
    if (result != SDL_TRUE) {
        char error[256];
        snprintf(error, 256, "Failed to get required instance extensions. Error: %s", SDL_GetError());
        throw std::runtime_error(error);
    }

#ifndef NDEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return extensions;
}

Instance::Instance() noexcept :
    instance{ nullptr }
{}

Instance::Instance(std::string_view app_name, const Window& window)
{
    const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = app_name.data(),
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Stealth Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3
    };

    const auto extensions = GetRequiredExtensions(window);

    const VkInstanceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };

    const auto result = vkCreateInstance(&info, nullptr, &instance);
    if (result != VK_SUCCESS) ThrowError("Failed to create instance.", result);
}

#ifndef NDEBUG
Instance::Instance(std::string_view app_name, std::span<const char* const> validation_layers, const Window& window)
{
    if (!CheckValidationLayerSupport(validation_layers))
        throw std::runtime_error("Validation layers requested but not available");

    const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = app_name.data(),
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Stealth Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3
    };

    const auto extensions = GetRequiredExtensions(window);

    const VkInstanceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(validation_layers.size()),
        .ppEnabledLayerNames = validation_layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };

    const auto result = vkCreateInstance(&info, nullptr, &instance);
    if (result != VK_SUCCESS) ThrowError("Failed to create instance.", result);
}
#endif

Instance::~Instance()
{
    vkDestroyInstance(instance, nullptr);
}

Instance::Instance(Instance&& i) noexcept :
    instance{ i.instance }
{
    i.instance = nullptr;
}

Instance& Instance::operator=(Instance&& i) noexcept
{
    vkDestroyInstance(instance, nullptr);
    instance = i.instance;
    i.instance = nullptr;
    return *this;
}
}