#pragma once
#include "backend/buffer_pool.h"
#include "backend/shader_module.h"
#include "rendering/render_context.h"

namespace xihe
{
class backend::CommandBuffer;

struct alignas(16) Light
{
	glm::vec4 position;         // position.w represents type of light
	glm::vec4 color;            // color.w represents light intensity
	glm::vec4 direction;        // direction.w represents range
	glm::vec2 info;             // (only used for spot lights) info.x represents light inner cone angle, info.y represents light outer cone angle
};

struct LightingState
{
	std::vector<Light> directional_lights;

	std::vector<Light> point_lights;

	std::vector<Light> spot_lights;

	backend::BufferAllocation light_buffer;
};

namespace rendering
{
	class Subpass
	{
	public:
	    Subpass(RenderContext &render_context, backend::ShaderSource &&vertex_shader, backend::ShaderSource &&fragment_shader);

		Subpass(const Subpass &) = delete;

	    Subpass(Subpass &&) = default;

	    virtual ~Subpass() = default;

	    Subpass &operator=(const Subpass &) = delete;

	    Subpass &operator=(Subpass &&) = delete;

	    /**
	     * @brief Prepares the shaders and shader variants for a subpass
	     */
	    virtual void prepare() = 0;

	    /**
	     * @brief Updates the render target attachments with the ones stored in this subpass
	     *        This function is called by the RenderPipeline before beginning the render
	     *        pass and before proceeding with a new subpass.
	     */
	    void update_render_target_attachments(RenderTarget &render_target);

	    /**
	     * @brief Draw virtual function
	     * @param command_buffer Command buffer to use to record draw commands
	     */
	    virtual void draw(backend::CommandBuffer &command_buffer);

	    RenderContext &get_render_context();

	    const backend::ShaderSource &get_vertex_shader() const;

	    const backend::ShaderSource &get_fragment_shader() const;

	    DepthStencilState &get_depth_stencil_state();

	    const std::vector<uint32_t> &get_input_attachments() const;

	    void set_input_attachments(std::vector<uint32_t> input);

	    const std::vector<uint32_t> &get_output_attachments() const;

	    void set_output_attachments(std::vector<uint32_t> output);

	    void set_sample_count(vk::SampleCountFlagBits sample_count);

	    const std::vector<uint32_t> &get_color_resolve_attachments() const;

	    void set_color_resolve_attachments(std::vector<uint32_t> color_resolve);

	    const bool &get_disable_depth_stencil_attachment() const;

	    void set_disable_depth_stencil_attachment(bool disable_depth_stencil);

	    const uint32_t &get_depth_stencil_resolve_attachment() const;

	    void set_depth_stencil_resolve_attachment(uint32_t depth_stencil_resolve);

	    vk::ResolveModeFlagBits get_depth_stencil_resolve_mode() const;

	    void set_depth_stencil_resolve_mode(vk::ResolveModeFlagBits mode);

	    LightingState &get_lighting_state();

	    const std::string &get_debug_name() const;

	    void set_debug_name(const std::string &name);

	protected:
	    RenderContext &render_context_;

		vk::SampleCountFlagBits sample_count_{vk::SampleCountFlagBits::e1};

		std::unordered_map<std::string, backend::ShaderResourceMode> resource_mode_map_;

		LightingState lighting_state_;

	private:
	    std::string debug_name_{};

		backend::ShaderSource vertex_shader_;

		backend::ShaderSource fragment_shader_;

		DepthStencilState depth_stencil_state_{};

		bool disable_depth_stencil_attachment_{false};

		vk::ResolveModeFlagBits depth_stencil_resolve_mode_{};

		std::vector<uint32_t> input_attachments_{};

		std::vector<uint32_t> output_attachments_{0};

		std::vector<uint32_t> color_resolve_attachments_{};

		uint32_t depth_stencil_resolve_attachment_{vk::AttachmentUnused};
	};
}

}
