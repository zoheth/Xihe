#pragma once

#include <memory>
#include <unordered_map>

#include "backend/debug.h"
#include "backend/device.h"
#include "backend/instance.h"
#include "backend/physical_device.h"
#include "platform/application.h"
#include "platform/window.h"
#include "rendering/render_context.h"
#include "rendering/render_pipeline.h"
#include "rendering/rdg_builder.h"
#include "scene_graph/scene.h"

namespace xihe
{

class XiheApp : public Application
{
  public:
	XiheApp();
	~XiheApp() override;

	bool prepare(Window *window) override;

	void update(float delta_time) override;

	void input_event(const InputEvent &input_event) override;

	void finish() override;

	const std::string &get_name() const;

	std::unique_ptr<backend::Device> const       &get_device() const;
	std::vector<const char *> const              &get_validation_layers() const;
	std::unordered_map<const char *, bool> const &get_device_extensions() const;
	std::unordered_map<const char *, bool> const &get_instance_extensions() const;
	std::unique_ptr<backend::Instance> const     &get_instance() const;

	rendering::RenderContext       &get_render_context();
	rendering::RenderContext const &get_render_context() const;
	bool                            has_render_context() const;

	using PostSceneUpdateCallback = std::function<void(float)>;

	void add_post_scene_update_callback(const PostSceneUpdateCallback &callback);

  protected:

	void load_scene(const std::string &path);

	void update_scene(float delta_time);

	// virtual std::unique_ptr<rendering::RenderTarget> create_render_target(backend::Image &&swapchain_image);

	static void set_viewport_and_scissor(backend::CommandBuffer const &command_buffer, vk::Extent2D const &extent);

	void update_bindless_descriptor_sets();

	void add_instance_extension(const char *extension, bool optional = false);
	void add_device_extension(const char *extension, bool optional = false);

	/**
	 * @brief Request features from the gpu based on what is supported
	 */
	virtual void request_gpu_features(backend::PhysicalDevice &gpu);

	void create_render_context();

  protected:
	std::unique_ptr<backend::Instance> instance_;

	std::unique_ptr<backend::Device> device_;

	std::unique_ptr<rendering::RenderContext> render_context_;

	std::unique_ptr<rendering::RdgBuilder> rdg_builder_;

	std::unique_ptr<sg::Scene> scene_;

	std::string name_{};

	uint32_t api_version_ = VK_API_VERSION_1_1;

	vk::SurfaceKHR surface_{};

	/** @brief Set of instance extensions to be enabled for this example and whether they are optional (must be set in the derived constructor) */
	std::unordered_map<const char *, bool> instance_extensions_;
	/** @brief Set of device extensions to be enabled for this example and whether they are optional (must be set in the derived constructor) */
	std::unordered_map<const char *, bool> device_extensions_;

	vk::PhysicalDeviceDescriptorIndexingPropertiesEXT descriptor_indexing_properties_{};

	std::vector<PostSceneUpdateCallback> post_scene_update_callbacks_{};
};
}        // namespace xihe
