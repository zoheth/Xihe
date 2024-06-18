#pragma once

#include "rendering/render_pipeline.h"
#include "rendering/render_target.h"

namespace xihe
{
namespace rendering
{
class RenderPipeline;
class RenderTarget;

class RdgPass
{
  public:
	RdgPass(const std::string &name, RenderContext &render_context);

	RdgPass(RdgPass &&) = default;

	virtual ~RdgPass() = default;

	/// \brief Creates or recreates a render target using a provided swapchain image.
	/// \param swapchain_image The image from the swapchain used to create the render target.
	/// This version of the function is called when the swapchain changes and a new image is available.
	virtual std::unique_ptr<RenderTarget> create_render_target(backend::Image &&swapchain_image);

	/// \brief Creates or recreates a render target without a swapchain image.
	/// This version is used when no swapchain image is available or needed.
	virtual RenderTarget *get_render_target() const;

	virtual void execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target) const = 0;

	virtual std::vector<vk::DescriptorImageInfo> get_descriptor_image_infos(RenderTarget &render_target) const;

	/// \brief Checks if the render target should be created using a swapchain image.
	/// \return True if a swapchain image should be used, otherwise false.
	bool use_swapchain_image() const;

  protected:

	std::string name_;

	RenderContext &render_context_;

	std::unique_ptr<RenderPipeline> render_pipeline_{nullptr};

	std::unique_ptr<RenderTarget>   render_target_{nullptr};

	bool use_swapchain_image_{true};
};
}        // namespace rendering
}        // namespace xihe
