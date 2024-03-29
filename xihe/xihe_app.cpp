﻿#include "xihe_app.h"

#include <cassert>

#include <volk.h>
#include <vulkan/vulkan.hpp>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include "backend/debug.h"
#include "common/error.h"
#include "common/logging.h"

namespace xihe
{
XiheApp::~XiheApp()
{}

bool XiheApp::prepare(Window *window)
{
	assert(window != nullptr && "Window must be valid");
	window_ = window;

	LOGI("XiheApp init");

	// initialize function pointers
	static vk::DynamicLoader dl;
	VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

	// for non-vulkan.hpp stuff, we need to initialize volk as well !!
	if (VkResult result = volkInitialize())
	{
		throw VulkanException(result, "Failed to initialize volk.");
	}

	// Creating the vulkan instance
	for (const char *extension_name : window->get_required_surface_extensions())
	{
		add_instance_extension(extension_name);
	}

	std::unique_ptr<backend::DebugUtils> debug_utils;
#ifdef XH_VULKAN_DEBUG
	{
		std::vector<vk::ExtensionProperties> available_extensions = vk::enumerateInstanceExtensionProperties();

		auto debug_extension_it = std::ranges::find_if(
		    available_extensions,
		    [](const vk::ExtensionProperties &extension) {
			    return strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, extension.extensionName) == 0;
		    });
		if (debug_extension_it != available_extensions.end())
		{
			LOGI("Vulkan debug utils enabled ({})", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

			debug_utils = std::make_unique<backend::DebugUtilsExtDebugUtils>();
			add_instance_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
	}
#endif

	instance_ = std::make_unique<backend::Instance>(get_name(), get_instance_extensions(), get_validation_layers(), api_version_);

	// Getting a valid vulkan surface from the platform
	surface_ = static_cast<vk::SurfaceKHR>(window_->create_surface(*instance_));
	if (!surface_)
	{
		throw std::runtime_error("Failed to create window surface.");
	}

	auto &gpu = instance_->get_suitable_gpu(surface_);
	// gpu.set_high_priority_graphics_queue_enable(high_priority_graphics_queue);

	request_gpu_features(gpu);

	add_device_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	if (instance_extensions_.contains(VK_KHR_DISPLAY_EXTENSION_NAME))
	{
		add_device_extension(VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME, /*optional=*/true);
	}

#ifdef XH_VULKAN_DEBUG
	{
		std::vector<vk::ExtensionProperties> available_extensions = gpu.get_handle().enumerateDeviceExtensionProperties();

		auto debug_extension_it = std::ranges::find_if(
		    available_extensions,
		    [](vk::ExtensionProperties const &ep) { return strcmp(ep.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0; });
		if (debug_extension_it != available_extensions.end())
		{
			LOGI("Vulkan debug utils enabled ({})", VK_EXT_DEBUG_MARKER_EXTENSION_NAME);

			debug_utils = std::make_unique<backend::DebugMarkerExtDebugUtils>();
			add_device_extension(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
		}
	}

	if (!debug_utils)
	{
		LOGW("Vulkan debug utils were requested, but no extension that provides them was found");
	}
#endif

	if (!debug_utils)
	{
		debug_utils = std::make_unique<backend::DummyDebugUtils>();
	}

	device_ = std::make_unique<backend::Device>(gpu, surface_, std::move(debug_utils), get_device_extensions());

	VULKAN_HPP_DEFAULT_DISPATCHER.init(get_device()->get_handle());

	create_render_context();
	prepare_render_context();

	return true;
}

void XiheApp::update(float delta_time)
{
	auto &command_buffer = render_context_
}

const std::string & XiheApp::get_name() const
{
	return name_;
}

std::unique_ptr<backend::Device> const &XiheApp::get_device() const
{
	return device_;
}

std::vector<const char *> const &XiheApp::get_validation_layers() const
{
	static std::vector<const char *> validation_layers;
	return validation_layers;
}

std::unordered_map<const char *, bool> const &XiheApp::get_device_extensions() const
{
	return device_extensions_;
}

std::unordered_map<const char *, bool> const &XiheApp::get_instance_extensions() const
{
	return instance_extensions_;
}

std::unique_ptr<backend::Instance> const &XiheApp::get_instance() const
{
	return instance_;
}

void XiheApp::add_instance_extension(const char *extension, bool optional)
{
	instance_extensions_[extension] = optional;
}

void XiheApp::add_device_extension(const char *extension, bool optional)
{
	device_extensions_[extension] = optional;
}

void XiheApp::request_gpu_features(backend::PhysicalDevice &gpu)
{
}

void XiheApp::create_render_context()
{
	auto surface_priority_list = std::vector<vk::SurfaceFormatKHR>{
	    {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
	    {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}};

	vk::PresentModeKHR              present_mode = (window_->get_properties().vsync == Window::Vsync::ON) ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eMailbox;
	std::vector<vk::PresentModeKHR> present_mode_priority_list{vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eImmediate};

	render_context_ = std::make_unique<rendering::RenderContext>(*get_device(), surface_, *window_, present_mode, present_mode_priority_list, surface_priority_list);
}

void XiheApp::prepare_render_context() const
{
	render_context_->prepare();
}
}        // namespace xihe

std::unique_ptr<xihe::Application> create_application()
{
	return std::make_unique<xihe::XiheApp>();
}
