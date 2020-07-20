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
#include <fstream>
#ifdef DEBUG
#include <magic_enum.hpp>
#endif
#ifdef WIN32
#include <Windows.h>
#include <mutex>
#elif defined(__linux__) && !defined(__ANDROID__)
#include <limits.h>
#include <unistd.h>
#include <mutex>
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
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
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

        swapChainImages = device.getSwapchainImagesKHR(swapchain);
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            vk::ImageViewCreateInfo createInfo{};
            createInfo.image = swapChainImages[i];
            createInfo.viewType = vk::ImageViewType::e2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = vk::ComponentSwizzle::eIdentity;
            createInfo.components.g = vk::ComponentSwizzle::eIdentity;
            createInfo.components.b = vk::ComponentSwizzle::eIdentity;
            createInfo.components.a = vk::ComponentSwizzle::eIdentity;
            createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0; // only relevant for stereographic apps
            createInfo.subresourceRange.layerCount = 1; // only relevant for stereographic apps
            swapChainImageViews[i] = device.createImageView(createInfo);
        }
    }

    void createRenderPass() {
        vk::AttachmentDescription colorAttachment{};
        colorAttachment.setFormat(swapChainImageFormat)
            .setSamples(vk::SampleCountFlagBits::e1)   // Not using multisampling yet
            .setLoadOp(vk::AttachmentLoadOp::eClear)   // clear operation to clear the framebuffer to black before drawing a new frame
            .setStoreOp(vk::AttachmentStoreOp::eStore) // Rendered contents will be stored in memory and can be read later
            .setInitialLayout(vk::ImageLayout::eUndefined) // The caveat of this special value is that the contents of the image are not guaranteed to be preserved, but that doesn't matter since we're going to clear it anyway.
            .setFinalLayout(vk::ImageLayout::ePresentSrcKHR); //  We want the image to be ready for presentation using the swap chain after rendering

        vk::AttachmentReference colorAttachmentRef{};
        colorAttachmentRef.setAttachment(0) // Our array consists of a single VkAttachmentDescription, so its index is 0
            .setLayout(vk::ImageLayout::eColorAttachmentOptimal); // use the attachment to function as a color buffer

        vk::SubpassDescription subpass{};
        subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics) // may also support compute subpasses in the future, so we have to be explicit about this being a graphics subpass
            .setColorAttachmentCount(1)
            .setPColorAttachments(&colorAttachmentRef); // The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive!

        vk::RenderPassCreateInfo renderPassInfo{};
        renderPassInfo.setAttachmentCount(1)
            .setPAttachments(&colorAttachment)
            .setSubpassCount(1)
            .setPSubpasses(&subpass);
        renderPass = device.createRenderPass(renderPassInfo);
    }

    void createGraphicsPipeline() {
        auto vertShaderCode = readFile(getShaderPath() + "/shader.vert.spv");
        auto fragShaderCode = readFile(getShaderPath() + "/shader.frag.spv");
        auto vertShaderModule = createShaderModule(vertShaderCode);
        auto fragShaderModule = createShaderModule(fragShaderCode);

        vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.setVertexBindingDescriptionCount(0)
                       .setVertexAttributeDescriptionCount(0);
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
        inputAssembly.primitiveRestartEnable = false;

        vk::Viewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vk::Rect2D scissor({0, 0}, swapChainExtent);

        vk::PipelineViewportStateCreateInfo viewportState({}, 1, &viewport, 1, &scissor);

        vk::PipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.setDepthClampEnable(false) // fragments that are beyond the near and far planes are clamped to them
            .setRasterizerDiscardEnable(false) // if true geometry never passes through the rasterizer stage. This basically disables any output to the framebuffer
            .setPolygonMode(vk::PolygonMode::eFill) // Using any mode other than fill requires enabling a GPU feature.
            .setLineWidth(1.0f)
            .setCullMode(vk::CullModeFlagBits::eBack)
            .setFrontFace(vk::FrontFace::eClockwise)
            .setDepthBiasEnable(false) //to alter the depth values by adding a constant value or biasing them based on a fragment's slope. This is sometimes used for shadow mapping.
            .setDepthBiasConstantFactor(0.0f)
            .setDepthBiasClamp(0.0f)
            .setDepthBiasSlopeFactor(0.0f);

        vk::PipelineMultisampleStateCreateInfo multisampling{};
        multisampling.setSampleShadingEnable(false) // configures multisampling, which is one of the ways to perform anti-aliasing
            .setRasterizationSamples(vk::SampleCountFlagBits::e1)
            .setMinSampleShading(1.0f)
            .setPSampleMask(nullptr)
            .setAlphaToCoverageEnable(false)
            .setAlphaToOneEnable(false);

        //TODO: Depth and stencial testing

        // color blending settings per framebuffer
        vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                               vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
            .setBlendEnable(false)
            .setSrcColorBlendFactor(vk::BlendFactor::eOne)
            .setDstColorBlendFactor(vk::BlendFactor::eZero)
            .setColorBlendOp(vk::BlendOp::eAdd)
            .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
            .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
            .setAlphaBlendOp(vk::BlendOp::eAdd);
        // Pseudocode for color blending:
        // if (blendEnable) {
        //     finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
        //     finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
        // } else {
        //     finalColor = newColor;
        // }
        // finalColor = finalColor & colorWriteMask;

        // global color blending settings -> for all framebuffers
        vk::PipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.setLogicOpEnable(false)
            .setLogicOp(vk::LogicOp::eCopy)
            .setAttachmentCount(1)
            .setPAttachments(&colorBlendAttachment)
            .setBlendConstants({0.0f, 0.0f, 0.0f, 0.0f});

        vk::DynamicState dynamicStates[] = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eLineWidth
        };
        vk::PipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.setDynamicStateCount(2)
            .setPDynamicStates(dynamicStates);

        // for uniform values in shaders
        // The structure also specifies push constants, 
        // which are another way of passing dynamic values to shaders.
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.setSetLayoutCount(0)
            .setPSetLayouts(nullptr)
            .setPushConstantRangeCount(0)
            .setPPushConstantRanges(nullptr);
        pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

        vk::GraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.setStageCount(2)
            .setPStages(shaderStages)
            .setPVertexInputState(&vertexInputInfo)
            .setPInputAssemblyState(&inputAssembly)
            .setPViewportState(&viewportState)
            .setPRasterizationState(&rasterizer)
            .setPMultisampleState(&multisampling)
            .setPDepthStencilState(nullptr)
            .setPColorBlendState(&colorBlending)
            .setPDynamicState(nullptr)
            .setLayout(pipelineLayout)
            .setRenderPass(renderPass) // It is also possible to use other render passes with this pipeline instead of this specific instance, but they have to be compatible
            .setSubpass(0)             // index of the sub pass where this graphics pipeline will be used
            // Vulkan allows you to create a new graphics pipeline by deriving from an existing pipeline.
            // The idea of pipeline derivatives is that it is less expensive to set up pipelines when they have much functionality
            // in common with an existing pipeline and switching between pipelines from the same parent can also be done quicker
            .setBasePipelineHandle(nullptr)
            .setBasePipelineIndex(-1);

        graphicsPipeline = device.createGraphicsPipeline({}, pipelineInfo);

        device.destroyShaderModule(vertShaderModule);
        device.destroyShaderModule(fragShaderModule);
    }

    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());
        for (size_t index = 0; index < swapChainImageViews.size(); index++)
        {
            vk::ImageView attachments[] = { swapChainImageViews[index] };
            vk::FramebufferCreateInfo frameBufferInfo{};
            frameBufferInfo.setRenderPass(renderPass) // specify with which renderPass needs to be compatible
                .setAttachmentCount(1)
                .setPAttachments(attachments)
                .setWidth(swapChainExtent.width)
                .setHeight(swapChainExtent.height)
                .setLayers(1); // Our swap chain images are single images, so the number of layers is 1
            swapChainFramebuffers[index] = device.createFramebuffer(frameBufferInfo);
        }
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
            
        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }

    static std::string getShaderPath() {
        return getExecutablePath() + "/shaders";
    }

    static std::string getExecutablePath() {
#if defined(_WIN32)
        static std::string path;
        static std::once_flag once;
        std::call_once(once, []{
            char result[MAX_PATH];
            std::string execPath(result, GetModuleFileName(nullptr, result, MAX_PATH));
            std::replace(execPath.begin(), execPath.end(), '\\', '/');
            std::string::size_type lastSlash = execPath.rfind("/");
            path = execPath.substr(0, lastSlash);
        });
        return path;
#elif defined(__linux__) && !defined(__ANDROID__)
        static std::string path;
        static std::once_flag once;
        std::call_once(once, []{
            char result[PATH_MAX];
            ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
            return std::string execPath(result, (count > 0) ? count : 0);
            std::replace(execPath.begin(), execPath.end(), '\\', '/');
            std::string::size_type lastSlash = execPath.rfind("/");
            path = execPath.substr(0, lastSlash);
        });
        return path;
#else
        static const std::string empty;
        return empty;
#endif
    }

    vk::ShaderModule createShaderModule(const std::vector<char>& code) {
        vk::ShaderModuleCreateInfo createInfo{};
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        return device.createShaderModule(createInfo);
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
        for (auto framebuffer : swapChainFramebuffers) {
            device.destroyFramebuffer(framebuffer);
        }
        device.destroyPipeline(graphicsPipeline);
        device.destroyPipelineLayout(pipelineLayout);
        device.destroyRenderPass(renderPass);
        for (auto imageView : swapChainImageViews) {
            device.destroyImageView(imageView);
        }
        device.destroySwapchainKHR(swapchain);
        instance.destroySurfaceKHR(surface);
        device.destroy();
#ifdef DEBUG
        instance.destroyDebugUtilsMessengerEXT(debugMessenger);
#endif
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
    std::vector<vk::Image> swapChainImages;
    vk::Format swapChainImageFormat;
    vk::Extent2D swapChainExtent;
    std::vector<vk::ImageView> swapChainImageViews;

    vk::RenderPass renderPass;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline graphicsPipeline;
    std::vector<vk::Framebuffer> swapChainFramebuffers;
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