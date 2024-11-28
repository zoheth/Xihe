#pragma once

#include "render_resource.h"
#include "render_graph.h"
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
	PassBuilder(GraphBuilder &graph_builder, const std::string &name) :
	    graph_builder_(graph_builder), pass_name_(name)
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

	PassBuilder &shader(const std::string &vert, const std::string &frag)
	{
		vert_shader_ = backend::ShaderSource{vert};
		frag_shader_ = backend::ShaderSource{frag};
		return *this;
	}

	template <typename F>
	void execute(F &&func)
	{
		execute_func_ = std::forward<F>(func);
		graph_builder_.add_pass(pass_name_, std::move(pass_info_),
		                        std::move(vert_shader_), std::move(frag_shader_),
		                        std::move(execute_func_));
	}

  private:
	GraphBuilder         &graph_builder_;
	std::string           pass_name_;
	PassInfo              pass_info_;

	std::optional<backend::ShaderSource> vertex_shader_;

	std::optional<backend::ShaderSource> task_shader_;
	std::optional<backend::ShaderSource> mesh_shader_;

	std::optional<backend::ShaderSource> fragment_shader_;

	std::optional<backend::ShaderSource> compute_shader_;

	std::function<void()> execute_func_;
};

class GraphBuilder
{
public:
	PassBuilder add_pass(const std::string &name)
	{
		return PassBuilder(*this, name);
	}

	// 内部方法,由PassBuilder调用
	void add_pass(const std::string      &name,
	              PassInfo              &&pass_info,
	              backend::ShaderSource &&vert_shader,
	              backend::ShaderSource &&frag_shader,
	              std::function<void()> &&execute_func)
	{
		// 存储pass信息
		passes_.emplace_back(PassData{
		    name,
		    std::move(pass_info),
		    std::move(vert_shader),
		    std::move(frag_shader),
		    std::move(execute_func)});
	}
};
}
