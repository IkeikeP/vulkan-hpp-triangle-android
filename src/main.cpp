//#define GLFW_INCLUDE_VULKAN
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define NOMINMAX
#include <vulkan/Vulkan.hpp>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <optional>
#include <set>
#include <cstdint>
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

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

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
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device) {
        QueueFamilyIndices indices;
        uint32_t index = 0;
        auto queueFamilies = device.getQueueFamilyProperties();
        
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
                indices.graphicsFamily = index;
            }
            vk::Bool32 presentSupport = device.getSurfaceSupportKHR(index, surface);
            if (presentSupport) {
                indices.presentFamily = index;
            }
            if (indices.isComplete()) {
                break;
            }
            index++;
        }
        return indices;
    }

    struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device) {
        SwapChainSupportDetails details;
        details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
        details.formats = device.getSurfaceFormatsKHR(surface);
        details.presentModes = device.getSurfacePresentModesKHR(surface);
        return details;
    }

    bool isSwapChainSupportSufficient(vk::PhysicalDevice device) {
        auto swapChainSupportDetails = querySwapChainSupport(device);
        return !swapChainSupportDetails.formats.empty() && !swapChainSupportDetails.presentModes.empty();
    }

    bool isDeviceSuitable(vk::PhysicalDevice device) {
        // Note: only check for swap chain support after verifying that the extension is avaible, therefore order must be kept
        return findQueueFamilies(device).isComplete() && checkDeviceExtensionSupport(device) && isSwapChainSupportSufficient(device);
    }

    bool checkDeviceExtensionSupport(vk::PhysicalDevice device) {
        auto availableExtensions = device.enumerateDeviceExtensionProperties();
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }
        return requiredExtensions.empty();
    }

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
    {
        for (const auto& availableFormat : availableFormats)
        {
            if (availableFormat.format == vk::Format::eB8G8R8A8Srgb
                && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return availableFormat;
            }
        }
        // we could start ranking the available formats based on how "good" they are, 
        // but in most cases it's okay to just settle with the first format that is specified.
        return availableFormats[0];
    }

    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
    {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
                return availablePresentMode;
            }
        }
        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        } else {
            vk::Extent2D actualExtent = {k_width, k_height};
            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
            return actualExtent;
        }
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
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
    }

    void createSurface() {
        VkSurfaceKHR temporarySurface = static_cast<VkSurfaceKHR>(surface);
        if (glfwCreateWindowSurface(instance, window, nullptr, &temporarySurface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
        surface = vk::SurfaceKHR(temporarySurface);
    }

    void pickPhysicalDevice() {
        auto devices = instance.enumeratePhysicalDevices();
        int bestScore = 0;
        vk::PhysicalDevice bestScoredDevice;
        for (const auto& device : devices) {
            int score = rateDeviceSuitability(device);
            if (score > bestScore) {
                bestScore = score;
                bestScoredDevice = device;
            }
        }

        if (!bestScoredDevice || bestScore == 0) {
            throw std::runtime_error("failed to find a suitable GPU!");
        } else {
            physicalDevice = bestScoredDevice;
            std::cout << "GPU:" << bestScoredDevice.getProperties().deviceName << "(" << bestScore << ")" << std::endl;
        }
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
        float queuePriority = 1.0f;
        for (uint32_t queueFamilyIndex : uniqueQueueFamilies) {
            vk::DeviceQueueCreateInfo queueCreateInfo({}, queueFamilyIndex, 1, &queuePriority);
            queueCreateInfos.push_back(queueCreateInfo);
        }
        vk::PhysicalDeviceFeatures deviceFeatures{};

        vk::DeviceCreateInfo createInfo{};
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
#ifdef DEBUG
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
#else
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
#endif

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        createInfo.pEnabledFeatures = &deviceFeatures;

        if (physicalDevice.createDevice(&createInfo, nullptr, &device) != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create logical device!");
        }
        graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
        presentQueue = device.getQueue(indices.presentFamily.value(), 0);
    }

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        auto surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        auto presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        auto extent = chooseSwapExtent(swapChainSupport.capabilities);
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && 
            imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }
        vk::SwapchainCreateInfoKHR createInfo{};
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1; // 1 unless when developing a stereoscopic 3D application
        createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = vk::SharingMode::eExclusive;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque; // alpha channel should not be used for blending with other windows
        createInfo.presentMode = presentMode;
        createInfo.clipped = true;
        createInfo.oldSwapchain = nullptr;
        swapchain = device.createSwapchainKHR(createInfo);
    }

    int rateDeviceSuitability(vk::PhysicalDevice device) {
        auto features = device.getFeatures();
        if (!features.geometryShader || !isDeviceSuitable(device))
            return 0;
        int score = 0;
        auto properties = device.getProperties();
        score += properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ? 1000 : 0;
        score += properties.limits.maxImageDimension2D;
        return score;
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
        device.destroySwapchainKHR(swapchain);
        instance.destroySurfaceKHR(surface);
        device.destroy();
        instance.destroy();
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    
    GLFWwindow* window;

    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debugMessenger;
    vk::SurfaceKHR surface;

    vk::PhysicalDevice physicalDevice;
    vk::Device device;

    vk::Queue graphicsQueue;
    vk::Queue presentQueue;

    vk::SwapchainKHR swapchain;
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