#include "xihe_app.h"

#include "scene_graph/components/texture.h"
#include "scene_graph/gltf_loader.h"
#include "scene_graph/script.h"

#include <cassert>

#include <volk.h>
#include <vulkan/vulkan.hpp>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include "backend/debug.h"
#include "common/error.h"
#include "common/logging.h"
#include "rendering/render_frame.h"

namespace xihe
{
XiheApp::XiheApp()
{
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
	add_device_extension(VK_KHR_MAINTENANCE3_EXTENSION_NAME);

	// Works around a validation layer bug with descriptor pool allocation with VARIABLE_COUNT.
	// See: https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/2350.
	add_device_extension(VK_KHR_MAINTENANCE1_EXTENSION_NAME);

	add_device_extension(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
	add_device_extension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
}

XiheApp::~XiheApp()
{
	if (device_)
	{
		device_->get_handle().waitIdle();
	}

	scene_.reset();

	rdg_builder_.reset();

	render_context_.reset();

	device_.reset();

	if (surface_)
	{
		instance_->get_handle().destroySurfaceKHR(surface_);
	}

	instance_.reset();
}

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
	// todo
	render_context_->prepare(8);

	rdg_builder_ = std::make_unique<rendering::RdgBuilder>(*render_context_);

	return true;
}

void XiheApp::update(float delta_time)
{
	update_scene(delta_time);

	for (const auto &callback : post_scene_update_callbacks_)
	{
		callback(delta_time);
	}

	// render_context_->begin();

	// command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	// update_bindless_descriptor_sets();

	rdg_builder_->execute();

	// command_buffer.end();

	// render_context_->submit(command_buffer);

	// render_context_->reset_bindless_index();
}

void XiheApp::input_event(const InputEvent &input_event)
{
	Application::input_event(input_event);

	if (scene_ && scene_->has_component<sg::Script>())
	{
		const auto scripts = scene_->get_components<sg::Script>();

		for (const auto script : scripts)
		{
			script->input_event(input_event);
		}
	}
}

void XiheApp::finish()
{
	Application::finish();
	if (device_)
	{
		device_->get_handle().waitIdle();
	}
}

const std::string &XiheApp::get_name() const
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

rendering::RenderContext & XiheApp::get_render_context()
{
	if (has_render_context())
	{
		return *render_context_;
	}
	else
	{
		throw std::runtime_error("Render context is not initialized.");
	}
}

rendering::RenderContext const & XiheApp::get_render_context() const
{
	if (has_render_context())
	{
		return *render_context_;
	}
	else
	{
		throw std::runtime_error("Render context is not initialized.");
	}
}

bool XiheApp::has_render_context() const
{
	return render_context_ != nullptr;
}

void XiheApp::add_post_scene_update_callback(const PostSceneUpdateCallback &callback)
{
	post_scene_update_callbacks_.push_back(callback);
}

void XiheApp::load_scene(const std::string &path)
{
	xihe::GltfLoader loader(*device_);

	scene_ = loader.read_scene_from_file(path);

	if (!scene_)
	{
		LOGE("Cannot load scene: {}", path.c_str());
		throw std::runtime_error("Cannot load scene: " + path);
	}
}

void XiheApp::update_scene(float delta_time)
{
	if (scene_)
	{
		if (scene_->has_component<sg::Script>())
		{
			const auto scripts = scene_->get_components<sg::Script>();

			for (const auto script : scripts)
			{
				script->update(delta_time);
			}
		}
	}
}

void XiheApp::update_bindless_descriptor_sets()
{
	if (scene_)
	{
		if (scene_->has_component<sg::BindlessTextures>())
		{
			const auto textures = scene_->get_components<sg::BindlessTextures>()[0]->get_textures();

			for (uint32_t i = 0; i < textures.size(); ++i)
			{
				vk::DescriptorImageInfo image_info = textures[i]->get_descriptor_image_info();

				render_context_->get_bindless_descriptor_set()->update(i, image_info);
			}
		}
	}
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
	gpu.get_mutable_requested_features().shaderSampledImageArrayDynamicIndexing = VK_TRUE;
	gpu.get_mutable_requested_features().depthClamp                             = VK_TRUE;

	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}

	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceDescriptorIndexingFeaturesEXT, shaderSampledImageArrayNonUniformIndexing);
	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceDescriptorIndexingFeaturesEXT, descriptorBindingSampledImageUpdateAfterBind);
	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceDescriptorIndexingFeaturesEXT, descriptorBindingStorageImageUpdateAfterBind);
	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceDescriptorIndexingFeaturesEXT, descriptorBindingPartiallyBound);
	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceDescriptorIndexingFeaturesEXT, descriptorBindingUpdateUnusedWhilePending);
	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceDescriptorIndexingFeaturesEXT, descriptorBindingVariableDescriptorCount);
	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceDescriptorIndexingFeaturesEXT, runtimeDescriptorArray);

	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceSynchronization2FeaturesKHR, synchronization2);
	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceTimelineSemaphoreFeatures, timelineSemaphore);
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

void XiheApp::set_viewport_and_scissor(backend::CommandBuffer const &command_buffer, vk::Extent2D const &extent)
{
	command_buffer.get_handle().setViewport(0, {{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f}});
	command_buffer.get_handle().setScissor(0, vk::Rect2D({}, extent));
}
}        // namespace xihe

// std::unique_ptr<xihe::Application> create_application()
//{
//	return std::make_unique<xihe::XiheApp>();
// }
