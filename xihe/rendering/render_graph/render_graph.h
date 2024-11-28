#pragma once
#include "backend/command_buffer.h"
#include "backend/sampler.h"
#include "rendering/render_context.h"
#include "rendering/render_target.h"

#include <functional>

namespace xihe::rendering
{
enum RenderResourceType
{
	kInvalid    = -1,
	kBuffer     = 0,
	kTexture    = 1,
	kAttachment = 2,
	kReference  = 3,
	kSwapchain  = 4
};

struct PassInput
{
	RenderResourceType type;
	std::string        name;
};
struct PassOutput
{
	RenderResourceType                                type;
	std::string                                       name;
	vk::Format                                        format;
	vk::ImageUsageFlags                               usage{};
	vk::Extent3D                                      override_resolution{};
	std::function<vk::Extent3D(const vk::Extent3D &)> modify_extent{};

	vk::AttachmentLoadOp load_op{vk::AttachmentLoadOp::eClear};
	backend::Sampler    *sampler{nullptr};

	void set_sampler(backend::Sampler *s)
	{
		sampler = s;
		usage |= vk::ImageUsageFlagBits::eSampled;
	}
};
struct PassInfo
{
	std::vector<PassInput>  inputs;
	std::vector<PassOutput> outputs;
};

enum PassType
{
	kNone    = 0,
	kRaster  = 1 << 0,
	kCompute = 1 << 1
};

class PassNode
{
  public:
	PassNode(std::string name, PassType type, PassInfo &&pass_info);

	void execute(backend::CommandBuffer &command_buffer);

	RenderTarget *get_render_target();

  private:
	void begin_execute(backend::CommandBuffer &command_buffer);

	void end_execute(backend::CommandBuffer &command_buffer);

	std::string name_;

	PassType type_;

	PassInfo pass_info_;

	std::unique_ptr<RenderTarget> render_target_;
};

class RenderGraph
{
  public:
	RenderGraph();

	void execute();

  private:
	struct PassBatch
	{
		std::vector<PassNode *> pass_nodes;
		PassType                type;
		int64_t                 wait_batch_index{-1};
		uint64_t                signal_semaphore_value{0};
	};

	void execute_raster_batch(PassBatch &pass_batch, bool is_first, bool is_last);

	void execute_compute_batch(PassBatch &pass_batch, bool is_first, bool is_last);

	RenderContext         &render_context_;

	std::vector<PassBatch> pass_batches_{};
};
}        // namespace xihe::rendering
