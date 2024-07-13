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
	virtual std::unique_ptr<RenderTarget> create_render_target(backend::Image &&swapchain_image) const;

	/// \brief Creates or recreates a render target without a swapchain image.
	/// This version is used when no swapchain image is available or needed.
	virtual RenderTarget *get_render_target() const;

	virtual void execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target, std::vector<backend::CommandBuffer *> secondary_command_buffers);

	virtual std::vector<vk::DescriptorImageInfo> get_descriptor_image_infos(RenderTarget &render_target) const;

	// before pall
	virtual void prepare(backend::CommandBuffer &command_buffer);

	virtual  void draw_subpass(backend::CommandBuffer &command_buffer, const RenderTarget &render_target, uint32_t i) const;

	backend::Device &get_device() const;

	std::vector<std::unique_ptr<Subpass>> &get_subpasses();

	uint32_t get_subpass_count() const;

	const std::vector<common::LoadStoreInfo> &get_load_store() const;

	backend::RenderPass &get_render_pass() const;
	backend::Framebuffer &get_framebuffer() const;

	/**
	 * @brief Thread index to use for allocating resources
	 */
	void set_thread_index(uint32_t subpass_index, uint32_t thread_index);

	/// \brief Checks if the render target should be created using a swapchain image.
	/// \return True if a swapchain image should be used, otherwise false.
	bool use_swapchain_image() const;


  protected:
	virtual void begin_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target, vk::SubpassContents contents = vk::SubpassContents::eInline);

	virtual void end_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target);

	void add_subpass(std::unique_ptr<Subpass> &&subpass);

	std::string name_;

	RenderContext &render_context_;

	// std::unique_ptr<RenderPipeline> render_pipeline_{nullptr};

	std::vector<std::unique_ptr<Subpass>> subpasses_;

	std::vector<common::LoadStoreInfo> load_store_;
	std::vector<vk::ClearValue> clear_value_;

	std::unique_ptr<RenderTarget>   render_target_{nullptr};

	bool use_swapchain_image_{true};

	backend::RenderPass *render_pass_{nullptr};
	backend::Framebuffer *framebuffer_{nullptr};
};
}        // namespace rendering
}        // namespace xihe
