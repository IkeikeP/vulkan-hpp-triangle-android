//#define GLFW_INCLUDE_VULKAN
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/Vulkan.hpp>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#ifdef DEBUG
#include <magic_enum.hpp>
#endif

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

static constexpr int k_width = 800;
static constexpr int k_height = 600;

#define LOG(x) std::cout << x << std::endl;

#ifdef DEBUG
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

#endif

class HelloTriangleApplication {
public:
    void run() {
#ifdef DEBUG
        std::cout << "DEBUG BUILD" << std::endl;
#endif
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

#ifdef DEBUG
    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        for (const auto* layerName : validationLayers) {
            bool layerFound = false;
            for (const auto& layerProperties : availableLayers) {
                if (!strcmp(layerName, layerProperties.layerName)) {
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound) {
                return false;
            }
        }
        return true;
    }
#endif

private:
    void initVulkan() {
        createInstance();
#ifdef DEBUG
        setupDebugMessenger();
#endif
        pickPhysicalDevice();
    }

    void pickPhysicalDevice() {
        auto devices = instance.enumeratePhysicalDevices();

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    bool isDeviceSuitable(vk::PhysicalDevice device) {
        return !!physicalDevice;
    }

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(k_width, k_height, "Vulkan", nullptr, nullptr);
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void createInstance() {
        using namespace vk;
        vk::DynamicLoader dl;
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
#ifdef DEBUG
        if (!checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }
#endif
        const ApplicationInfo appInfo = getApplicationInfo();
        auto extensions = getRequiredExtensions();
        InstanceCreateInfo createInfo(vk::InstanceCreateFlags(), 
                                      &appInfo,
#ifdef DEBUG
                                      static_cast<uint32_t>(validationLayers.size()),
                                      validationLayers.data(),
#else
                                      0,
                                      nullptr,
#endif
                                      static_cast<uint32_t>(extensions.size()),
                                      extensions.data());
#ifdef DEBUG
        vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = createDebugMessenger();
        createInfo.setPNext(&debugCreateInfo);
#endif

        instance = vk::createInstance(createInfo);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
    }
    
#ifdef DEBUG
    void setupDebugMessenger() {
        vk::DebugUtilsMessengerCreateInfoEXT createInfo = createDebugMessenger();
        debugMessenger = instance.createDebugUtilsMessengerEXT(createInfo);
    }

    vk::DebugUtilsMessengerCreateInfoEXT createDebugMessenger() {
        return vk::DebugUtilsMessengerCreateInfoEXT(vk::DebugUtilsMessengerCreateFlagsEXT(), 
                                                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
                                                    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                                                    debugCallback);
    }
#endif

    vk::ApplicationInfo getApplicationInfo() {
        return vk::ApplicationInfo("Hello Triangle", VK_MAKE_VERSION(1, 0, 0), "No Engine", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_0);
    }

    std::vector<const char*> getRequiredExtensions() {
        uint32_t extensionCount = 0, glfwExtensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::cout << "Extensions:" << glfwExtensionCount << std::endl;
        for (const auto& extension : availableExtensions) {
            bool enabled = false;
            for (auto i = 0u; i < glfwExtensionCount; ++i) {
                if (!strcmp(extension.extensionName, glfwExtensions[i])) {
                    enabled = true;
                    break;
                }
            }
            std::cout << "\t" << ((enabled) ? " [X] " : " [ ] ") << extension.extensionName << std::endl;
        }

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#ifdef DEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
        return extensions;
    }

    void cleanup() {
#ifdef DEBUG
        instance.destroyDebugUtilsMessengerEXT(debugMessenger);
#endif

        instance.destroy();
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    
    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    vk::DebugUtilsMessengerEXT debugMessenger;
    GLFWwindow* window;
};

int main() {
    try {
        HelloTriangleApplication app {};
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error:" << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}