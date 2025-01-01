#pragma once

#include "backend/sampler.h"
#include "backend/shader_module.h"
#include "render_graph.h"
#include "render_resource.h"
#include "rendering/passes/render_pass.h"

#include <functional>
#include <optional>

namespace xihe::rendering
{

class ResourceStateTracker
{
  public:
	struct State
	{
		ResourceUsageState  usage_state;
		int32_t             producer_pass{-1};
		int32_t             last_user{-1};
		backend::ImageView *image_view{nullptr};
	};
	State get_or_create_state(const ResourceHandle &handle);

	void track_resource(const ResourceHandle &handle, uint32_t node, const ResourceUsageState &state);

  private:
	std::unordered_map<ResourceHandle, State> states_;
};

struct ResourceCreateInfo
{
	bool is_buffer   = false;
	bool is_external = false;
	bool is_handled  = false;        // Track if resource has been created

	vk::BufferUsageFlags buffer_usage{};
	uint32_t             buffer_size{0};

	vk::ImageUsageFlags  image_usage{};
	vk::ImageCreateFlags image_flags{};
	vk::Format           format = vk::Format::eUndefined;
	ExtentDescriptor     extent_desc{};
	uint32_t             array_layers{1};
};

class GraphBuilder
{
  public:
	class PassBuilder
	{
	  public:
		PassBuilder(GraphBuilder &graph_builder, std::string name, std::unique_ptr<RenderPass> &&render_pass);

		PassBuilder &bindables(std::initializer_list<PassBindable> bindables);
		PassBuilder &attachments(std::initializer_list<PassAttachment> attachments);
		PassBuilder &shader(std::initializer_list<std::string> file_names);

		PassBuilder &bindables(const std::vector<PassBindable> &bindables);
		PassBuilder &attachments(const std::vector<PassAttachment> &attachments);

		PassBuilder &present();

		PassBuilder &gui(Gui *gui);

		void finalize();

	  private:
		GraphBuilder               &graph_builder_;
		std::string                 pass_name_;
		PassInfo                    pass_info_;
		std::unique_ptr<RenderPass> render_pass_;
		bool                        is_present_{false};
		Gui                        *gui_{nullptr};
	};

	GraphBuilder(RenderGraph &render_graph, RenderContext &render_context);

	PassBuilder add_pass(const std::string &name, std::unique_ptr<RenderPass> &&render_pass)
	{
		return {*this, name, std::move(render_pass)};
	}
	void build();

	void recreate_resources();

  private:
	class PassBatchBuilder
	{
	  public:
		void process_pass(PassNode *pass);

		void set_batch_dependency(int64_t wait_batch_index);

		std::vector<PassBatch> finalize();

	  private:
		void finalize_current_batch();

		PassBatch              current_batch_{};
		std::vector<PassBatch> batches_;
	};

	void add_pass(const std::string            &name,
	              PassInfo                    &&pass_info,
	              std::unique_ptr<RenderPass> &&render_pass,
	              bool                          is_present,
	              Gui                          *gui = nullptr);

	void create_resources();

	void collect_resource_create_info();

	void create_graph_resource();

	void build_pass_batches();

	std::pair<std::vector<std::unordered_set<uint32_t>>, std::vector<uint32_t>>
	    build_dependency_graph() const;

	void process_pass_resources(
	    uint32_t              node,
	    PassNode             &pass,
	    ResourceStateTracker &tracker,
	    PassBatchBuilder     &batch_builder);

	RenderGraph   &render_graph_;
	RenderContext &render_context_;

	std::unordered_map<std::string, ResourceCreateInfo> resource_create_infos_;

	std::unordered_map<std::string, ResourceHandle> resource_handles_;

	bool is_dirty_{false};
};
}        // namespace xihe::rendering
