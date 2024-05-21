#pragma once

#include <string>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace xihe::backend
{

class PhysicalDevice;

class Instance
{
  public:
	Instance(const std::string                            &application_name,
	         const std::unordered_map<const char *, bool> &required_extensions        = {},
	         const std::vector<const char *>              &required_validation_layers = {},
	         uint32_t                                      api_version                = VK_API_VERSION_1_0);
	Instance(vk::Instance instance);

	Instance(const Instance &) = delete;
	Instance(Instance &&)      = delete;

	~Instance();

	Instance &operator=(const Instance &) = delete;
	Instance &operator=(Instance &&)      = delete;

	vk::Instance get_handle() const;

	PhysicalDevice &get_suitable_gpu(vk::SurfaceKHR) const;

	bool is_enabled(const char *extension) const;

  private:
	void query_gpus();

  private:
	vk::Instance handle_;

	std::vector<const char *> enabled_extensions_;

	std::vector<std::unique_ptr<PhysicalDevice>> gpus_;

#if defined(XH_DEBUG) || defined(XH_VALIDATION_LAYERS)
	vk::DebugUtilsMessengerEXT debug_utils_messenger_;
	vk::DebugReportCallbackEXT debug_report_callback_;
#endif
};

}        // namespace xihe::backend
