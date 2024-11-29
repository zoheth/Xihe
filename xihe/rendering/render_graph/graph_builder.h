#pragma once

#include "render_resource.h"
#include "render_graph.h"
#include "rendering/passes/render_pass.h"
#include "backend/sampler.h"
#include "backend/shader_module.h"

#include <functional>
#include <optional>

namespace xihe::rendering
{
class GraphBuilder;

class PassBuilder
{
public:
	PassBuilder(GraphBuilder &graph_builder, std::string name, RenderPass &&render_pass) :
	  graph_builder_(graph_builder), pass_name_(std::move(name)), render_pass_(std::move(render_pass))
	{}

	PassBuilder &inputs(std::initializer_list<PassInput> inputs)
	{
		pass_info_.inputs = inputs;
		return *this;
	}

	PassBuilder &outputs(std::initializer_list<PassOutput> outputs)
	{
		pass_info_.outputs = outputs;
		return *this;
	}

	PassBuilder &shader(std::initializer_list<std::string> file_names);

	void finish();

private:
	GraphBuilder         &graph_builder_;
	std::string           pass_name_;
	PassInfo              pass_info_;
	RenderPass            render_pass_;


};

class GraphBuilder
{
public:
	GraphBuilder(RenderContext &render_context) :
	    render_context_(render_context)
	{}

	PassBuilder add_pass(const std::string &name, RenderPass &&render_pass)
	{
		return {*this, name, std::move(render_pass)};
	}

	// 内部方法,由PassBuilder调用
	void add_pass(const std::string &name,
	              PassInfo         &&pass_info,
	              RenderPass       &&render_pass);

	void build();

private:
	RenderContext &render_context_;
	
	bool is_dirty_{false};
};
}
