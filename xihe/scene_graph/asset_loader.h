#pragma once

#include "backend/command_buffer.h"
#include "components/texture.h"
#include "scene.h"

namespace xihe
{

void upload_image_to_gpu(
    backend::CommandBuffer &      command_buffer,
    const backend::Buffer        &staging_buffer,
    sg::Image              &      image,
    vk::ImageLayout               final_layout = vk::ImageLayout::eShaderReadOnlyOptimal,
    vk::PipelineStageFlags2       final_stage  = vk::PipelineStageFlagBits2::eFragmentShader,
    vk::AccessFlags2              final_access = vk::AccessFlagBits2::eShaderRead);

class AssetLoader
{
  public:
	AssetLoader(backend::Device &device);
	sg::Texture *load_texture_cube(sg::Scene &scene, const std::string &name, const std::string &file_name) const;

  private:
	backend::Device &device_;
};
}        // namespace xihe
