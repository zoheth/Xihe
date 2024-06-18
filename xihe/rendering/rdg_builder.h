#pragma once

#include "rdg_pass.h"
#include "render_context.h"

namespace xihe::rendering
{
class RdgBuilder
{
  public:
	explicit RdgBuilder(RenderContext &render_context);

	template <typename T, typename... Args>
	void add_pass(std::string name, Args &&...args);

	void execute(backend::CommandBuffer &command_buffer) const;

  private:

	static void set_viewport_and_scissor(backend::CommandBuffer const &command_buffer, vk::Extent2D const &extent);

	RenderContext &render_context_;
	std::unordered_map<std::string, std::unique_ptr<RdgPass>> rdg_passes_{};
};

template <typename T, typename ... Args>
void RdgBuilder::add_pass(std::string name, Args &&... args)
{
	static_assert(std::is_base_of_v<rendering::RdgPass, T>, "T must be a derivative of RenderPass");

	if (rdg_passes_.contains(name))
	{
		throw std::runtime_error{"Pass with name " + name + " already exists"};
	}

	rdg_passes_.emplace(name, std::make_unique<T>(name, render_context_, std::forward<Args>(args)...));

	if (rdg_passes_[name]->use_swapchain_image())
	{
		render_context_.register_rdg_render_target(name, 
			[name, this](backend::Image &&swapchain_image) {
				return rdg_passes_[name]->create_render_target(std::move(swapchain_image));
			});
	}


}
}        // namespace xihe::rendering
