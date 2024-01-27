#pragma once

#include <memory>
#include <unordered_map>

#include "backend/debug.h"
#include "backend/device.h"
#include "backend/physical_device.h"
#include "backend/instance.h"
#include "platform/window.h"
#include "platform/application.h"
#include "rendering/render_context.h"

namespace xihe
{

class XiheApp : public Application
{
  public:
	XiheApp() = default;
	~XiheApp();

	bool prepare(Window *window) override;

	void update(float delta_time) override;

	const std::string &get_name() const;

	std::unique_ptr<backend::Device> const       &get_device() const;
	std::vector<const char *> const              &get_validation_layers() const;
	std::unordered_map<const char *, bool> const &get_device_extensions() const;
	std::unordered_map<const char *, bool> const &get_instance_extensions() const;
	std::unique_ptr<backend::Instance> const     &get_instance() const;

  private:

	void add_instance_extension(const char *extension, bool optional = false);
	void add_device_extension(const char *extension, bool optional = false);

	/**
	 * @brief Request features from the gpu based on what is supported
	 */
	virtual void request_gpu_features(backend::PhysicalDevice &gpu);

	void create_render_context();
	void prepare_render_context() const;

  private:
	std::unique_ptr<backend::Instance> instance_;

	std::unique_ptr<backend::Device> device_;

	std::unique_ptr<rendering::RenderContext> render_context_;

	std::string name_{};

	uint32_t                               api_version_ = VK_API_VERSION_1_0;

	vk::SurfaceKHR surface_{};

	/** @brief Set of instance extensions to be enabled for this example and whether they are optional (must be set in the derived constructor) */
	std::unordered_map<const char *, bool> instance_extensions_;
	/** @brief Set of device extensions to be enabled for this example and whether they are optional (must be set in the derived constructor) */
	std::unordered_map<const char *, bool> device_extensions_;

};
}        // namespace xihe
