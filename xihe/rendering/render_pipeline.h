#pragma once

#include "rendering/subpass.h"

namespace xihe::rendering
{
	class RenderPipeline
	{
	public:
	    RenderPipeline(std::vector<std::unique_ptr<Subpass>> &&subpasses = {});

	    RenderPipeline(const RenderPipeline &) = delete;

	    RenderPipeline(RenderPipeline &&) = default;

	    virtual ~RenderPipeline() = default;

	    RenderPipeline &operator=(const RenderPipeline &) = delete;

	    RenderPipeline &operator=(RenderPipeline &&) = default;

	    void prepare();

	    const std::vector<common::LoadStoreInfo> &get_load_store() const;

	    void set_load_store(const std::vector<common::LoadStoreInfo> &load_store);

	    const std::vector<vk::ClearValue> &get_clear_value() const;

	    void set_clear_value(const std::vector<vk::ClearValue> &clear_values);

	    void add_subpass(std::unique_ptr<Subpass> &&subpass);

	    std::vector<std::unique_ptr<Subpass>> &get_subpasses();

	    /**
	     * @brief Record draw commands for each Subpass
	     */
	    void draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target, vk::SubpassContents contents = vk::SubpassContents::eInline);


	    /**
	     * @return Subpass currently being recorded, or the first one
	     *         if drawing has not started
	     */
	    std::unique_ptr<Subpass> &get_active_subpass();

	  private:
	    std::vector<std::unique_ptr<Subpass>> subpasses_;

	    /// Default to two load store
	    std::vector<common::LoadStoreInfo> load_store_ = std::vector<common::LoadStoreInfo>(2);

	    /// Default to two clear values
	    std::vector<vk::ClearValue> clear_value_ = std::vector<vk::ClearValue>(2);

	    size_t active_subpass_index_{0};
	};
}
