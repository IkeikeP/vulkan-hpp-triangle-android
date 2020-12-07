#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define NOMINMAX
#ifdef __ANDROID__
#include <android/asset_manager.h>
#include <jni.h>
#include <android/asset_manager_jni.h>
#include "vulkan-wrapper-patch.h"
#include <vulkan_wrapper.h>
#undef VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan.hpp>
#include "SDL.h"
#include <SDL_vulkan.h>

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

#ifndef __ANDROID__
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

static constexpr int k_width = 800;
static constexpr int k_height = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

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
            int width, height;
            SDL_GetWindowSize(window, &width, &height);
            vk::Extent2D actualExtent = {(uint32_t)width, (uint32_t)height};
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
#ifdef __ANDROID__
        // Try to dynamically load the Vulkan library and seed the function pointer mapping.
        if (!InitVulkan())
        {
            return;
        }
#endif
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
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
    }

    void createSurface() {
        VkSurfaceKHR temporarySurface;

        // Ask SDL to create a Vulkan surface from its window.
        if (!SDL_Vulkan_CreateSurface(window, instance, &temporarySurface))
        {
            throw std::runtime_error("SDL could not create a Vulkan surface.");
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

        vk::SubpassDependency dependency{};
        dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL) // VK_SUBPASS_EXTERNAL means anything outside of a given render pass scope, it specifies anything that happened before the render pass
            .setDstSubpass(0) //subpass index
            //The next two fields specify the operations to wait on and the stages in which these operations occur
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            //.setSrcAccessMask(0)
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

        vk::RenderPassCreateInfo renderPassInfo{};
        renderPassInfo.setAttachmentCount(1)
            .setPAttachments(&colorAttachment)
            .setSubpassCount(1)
            .setPSubpasses(&subpass)
            .setDependencyCount(1)
            .setPDependencies(&dependency);
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

        //TODO: Depth and stencil testing

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

    void createCommandPool() {
        auto queueFamiliesIndices = findQueueFamilies(physicalDevice);
        vk::CommandPoolCreateInfo poolInfo{};
        poolInfo.setQueueFamilyIndex(queueFamiliesIndices.graphicsFamily.value());
        commandPool = device.createCommandPool(poolInfo);
    }

    void createCommandBuffers() {
        commandBuffers.resize(swapChainFramebuffers.size());
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.setCommandPool(commandPool)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount((uint32_t)commandBuffers.size());
        commandBuffers = device.allocateCommandBuffers(allocInfo);
        // CBLevel::ePrimary: Can be submitted to a queue for execution, but cannot be called from other command buffers.
        // CBLevel::eSecondary: Cannot be submitted directly, but can be called from primary command buffers.
        for (size_t index = 0; index < commandBuffers.size(); index++) {
            vk::CommandBufferBeginInfo beginInfo{};
            // eOneTimeSubmit: specifies that each recording of the command buffer will only be submitted once, and the command buffer will be reset and recorded again between each submission
            // eRenderPassContinue: This is a secondary command buffer that will be entirely within a single render pass.
            // eSimultaneousUse: The command buffer can be resubmitted while it is also already pending execution.
            // None of these flags are applicable for us right now.
            commandBuffers[index].begin(beginInfo);
            
            vk::RenderPassBeginInfo renderPassInfo{};
            vk::ClearValue clearColor(std::array<float, 4> {0.0f, 0.0f, 0.0f, 1.0f});
            renderPassInfo.setRenderPass(renderPass)
                .setFramebuffer(swapChainFramebuffers[index])
                .setRenderArea({{0, 0}, swapChainExtent}) // Size of the render area. The render area defines where shader loads and stores will take place. It should match the size of the attachments for best performance
                .setClearValueCount(1)
                .setPClearValues(&clearColor); // clear values for AttachmentLoadOp::eClear
            commandBuffers[index].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
            // SubpassContents::eInline: The render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed.
            // SubpassContents::eSecondaryCommandBuffers: The render pass commands will be executed from secondary command buffers.

            commandBuffers[index].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline); // first parameter specifies if is a graphics or compute pipeline
            commandBuffers[index].draw(3, 1, 0, 0);            
            // vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
            // instanceCount: Used for instanced rendering, use 1 if you're not doing that.
            // firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
            // firstInstance: Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.

            commandBuffers[index].endRenderPass();
            commandBuffers[index].end();
        }
    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.clear();
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        imagesInFlight.clear();
        imagesInFlight.resize(swapChainImages.size(), nullptr);
        vk::SemaphoreCreateInfo semaphoreInfo{};
        vk::FenceCreateInfo fenceInfo{};
        fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            imageAvailableSemaphores[i] = device.createSemaphore(semaphoreInfo);
            renderFinishedSemaphores[i] = device.createSemaphore(semaphoreInfo);
            inFlightFences[i] = device.createFence(fenceInfo);
        }
    }

    static std::vector<char> readFile(const std::string& filename) {
#ifdef __ANDROID__
        JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();  // Pointer to native interface
        jint ver = env->GetVersion();
        jobject activity = (jobject)SDL_AndroidGetActivity();
        jclass clazz(env->GetObjectClass(activity));
        jmethodID midGetContext = env->GetStaticMethodID(clazz, "getContext", "()Landroid/content/Context;");
        auto context = env->CallStaticObjectMethod(clazz, midGetContext);
        auto mid = env->GetMethodID(env->GetObjectClass(context), "getAssets", "()Landroid/content/res/AssetManager;");
        jobject ez = env->CallObjectMethod(context, mid);
        
        AAssetManager *assetManager = AAssetManager_fromJava(env, ez);
        AAsset* asset = AAssetManager_open(assetManager, filename.c_str(), AASSET_MODE_STREAMING);
        size_t size = AAsset_getLength(asset);
        std::vector<char> buffer(size);
        AAsset_read(asset, buffer.data(), size);
        AAsset_close(asset);
        return buffer;
#else
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
#endif
    }

    static std::string getShaderPath() {
#if defined(__ANDROID__)
        return "shaders";
#else
        return getExecutablePath() + "/shaders";
#endif
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
#elif defined(__ANDROID__)
        return "";
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
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
		    std::cerr << "Failed to initialize SDL:" << SDL_GetError() << std::endl;
            throw std::runtime_error("Failed to initialize SDL!");
	    }
        window = SDL_CreateWindow(
            "A Simple Triangle",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            k_width, k_height,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI);
#ifdef __ANDROID__
        SDL_SetWindowFullscreen(window, SDL_TRUE);
#else
        SDL_SetWindowFullscreen(window, SDL_FALSE);
#endif

    }

    void onWindowResize() {
        framebufferResized = true;
    }

    void mainLoop() {
        while (true) {
            SDL_Event event;
            // Each loop we will process any events that are waiting for us.
            bool quit = false;
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                    case SDL_WINDOWEVENT:
                        //onWindowResize();
                        break;
                    case SDL_RENDER_DEVICE_RESET:
                        smartRecreate();
                        break;

                    case SDL_QUIT:
                        quit = true;

                    case SDL_KEYDOWN:
                        if (event.key.keysym.sym == SDLK_ESCAPE) {
                            quit = true;
                        }
                        break;
                    default:
                        break;
                }
            }
            if (quit) {
                break;
            }
            drawFrame();
        }
        device.waitIdle();
    }

    void drawFrame() {
        device.waitForFences(1, &inFlightFences[currentFrame], true, UINT64_MAX);
        uint32_t imageIndex;

        if (framebufferResized)
            recreateSwapChain();
        // acquireNextImageKHR will signal semaphore when complete
        auto result = device.acquireNextImageKHR(swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], nullptr);
        if (result.result == vk::Result::eErrorOutOfDateKHR) {
            recreateSwapChain();
            return;
        }
        imageIndex = result.value;
        // Check if a previous frame is using this image (i.e. there is its fence to wait on)
        if (imagesInFlight[imageIndex]) {
            device.waitForFences(1, &imagesInFlight[imageIndex], true, UINT64_MAX);
        }
        // Mark the image as now being in use by this frame
        imagesInFlight[imageIndex] = inFlightFences[currentFrame];
        vk::SubmitInfo submitInfo{};
        vk::Semaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        vk::PipelineStageFlags waitStages(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        // Specify which semaphores to wait on before execution begins and in which stage(s) of the pipeline to wait
        vk::Semaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.setWaitSemaphoreCount(1)
            .setPWaitSemaphores(waitSemaphores)
            .setPWaitDstStageMask(&waitStages)
            .setCommandBufferCount(1)
            .setPCommandBuffers(&commandBuffers[imageIndex])
            .setSignalSemaphoreCount(1)
            .setPSignalSemaphores(signalSemaphores);

        device.resetFences(1, &inFlightFences[currentFrame]);

        graphicsQueue.submit(1, &submitInfo, inFlightFences[currentFrame]);

        vk::SwapchainKHR swapChains[] = {swapchain};
        vk::PresentInfoKHR presentInfo{};
        presentInfo.setWaitSemaphoreCount(1)
            .setPWaitSemaphores(signalSemaphores)
            .setSwapchainCount(1)
            .setPSwapchains(swapChains)
            .setPImageIndices(&imageIndex)
            .setPResults(nullptr); // only relevant for multiple swapchains

        auto presentResult = presentQueue.presentKHR(&presentInfo);
        if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
            return;
        }
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void recreateSwapChain()
    {
        int width = 0, height = 0;
        SDL_GetWindowSize(window, &width, &height);
        if (width == 0 || height == 0)
        {
            LOG("AAAA");
        }
        device.waitIdle();

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandBuffers();
    }

    void createInstance() {
#ifndef __ANDROID__
        vk::DynamicLoader dl;
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
#endif
#ifdef DEBUG
        if (!checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }
#endif
        const vk::ApplicationInfo appInfo = getApplicationInfo();
        auto extensions = getRequiredExtensions();
        vk::InstanceCreateInfo createInfo(vk::InstanceCreateFlags(), 
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
#ifndef __ANDROID__
        VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
#endif
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
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
            
        uint32_t sdlExtensionCount;
        SDL_Vulkan_GetInstanceExtensions(nullptr, &sdlExtensionCount, nullptr);

        auto sdlExtensionNames{std::make_unique<const char**>(new const char*[sdlExtensionCount])};
        SDL_Vulkan_GetInstanceExtensions(nullptr, &sdlExtensionCount, *sdlExtensionNames);
        std::vector<const char*> sdlExtensions(*sdlExtensionNames, *sdlExtensionNames + sdlExtensionCount);


        std::cout << "Extensions:" << sdlExtensionCount << std::endl;
        for (const auto& extension : availableExtensions) {
            bool enabled = false;
            for (auto i = 0u; i < sdlExtensionCount; ++i) {
                if (!strcmp(extension.extensionName, (*sdlExtensionNames)[i])) {
                    enabled = true;
                    break;
                }
            }
            std::cout << "\t" << ((enabled) ? " [X] " : " [ ] ") << extension.extensionName << std::endl;
        }
#ifdef DEBUG
        sdlExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
        return sdlExtensions;
    }

    void cleanup() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            device.destroySemaphore(renderFinishedSemaphores[i]);
            device.destroySemaphore(imageAvailableSemaphores[i]);
            device.destroyFence(inFlightFences[i]);
        }
        cleanupSwapChain();
        device.destroyCommandPool(commandPool);
        instance.destroySurfaceKHR(surface);
        device.destroy();
#ifdef DEBUG
        instance.destroyDebugUtilsMessengerEXT(debugMessenger);
#endif
        instance.destroy();
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void smartRecreate() {
        device.waitIdle();
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            device.destroySemaphore(renderFinishedSemaphores[i]);
            device.destroySemaphore(imageAvailableSemaphores[i]);
            device.destroyFence(inFlightFences[i]);
        }
        cleanupSwapChain();
        device.destroyCommandPool(commandPool);
        instance.destroySurfaceKHR(surface);
        device.destroy();

        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
    }
    
    void cleanupSwapChain()
    {
        for (auto framebuffer : swapChainFramebuffers) {
            device.destroyFramebuffer(framebuffer);
        }
        device.freeCommandBuffers(commandPool, commandBuffers);
        device.destroyPipeline(graphicsPipeline);
        device.destroyPipelineLayout(pipelineLayout);
        device.destroyRenderPass(renderPass);
        for (auto imageView : swapChainImageViews) {
            device.destroyImageView(imageView);
        }
        device.destroySwapchainKHR(swapchain);
    }
    
    SDL_Window* window;

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
    std::vector<vk::Framebuffer> swapChainFramebuffers;

    vk::RenderPass renderPass;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline graphicsPipeline;

    vk::CommandPool commandPool;
    std::vector<vk::CommandBuffer> commandBuffers;

    std::vector<vk::Semaphore> imageAvailableSemaphores;
    std::vector<vk::Semaphore> renderFinishedSemaphores;
    std::vector<vk::Fence> inFlightFences;
    std::vector<vk::Fence> imagesInFlight;
    
    size_t currentFrame = 0;
    bool framebufferResized = false;
};

int SDL_main(int, char* []) {
    try {
        HelloTriangleApplication app {};
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error:" << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}