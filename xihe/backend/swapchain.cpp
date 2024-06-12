#include "swapchain.h"

#include "backend/device.h"
#include "common/logging.h"

namespace
{
template <class T>
constexpr const T &clamp(const T &v, const T &lo, const T &hi)
{
	return (v < lo) ? lo : ((hi < v) ? hi : v);
}

vk::Extent2D choose_extent(vk::Extent2D        request_extent,
                           const vk::Extent2D &min_image_extent,
                           const vk::Extent2D &max_image_extent,
                           const vk::Extent2D &current_extent)
{
	if (current_extent.width == 0xFFFFFFFF)
	{
		return request_extent;
	}

	if (request_extent.width < 1 || request_extent.height < 1)
	{
		LOGW("(Swapchain) Image extent ({}, {}) not supported. Selecting ({}, {}).",
		     request_extent.width,
		     request_extent.height,
		     current_extent.width,
		     current_extent.height);
		return current_extent;
	}

	request_extent.width  = clamp(request_extent.width, min_image_extent.width, max_image_extent.width);
	request_extent.height = clamp(request_extent.height, min_image_extent.height, max_image_extent.height);

	return request_extent;
}

vk::PresentModeKHR choose_present_mode(vk::PresentModeKHR                     request_present_mode,
                                       const std::vector<vk::PresentModeKHR> &available_present_modes,
                                       const std::vector<vk::PresentModeKHR> &present_mode_priority_list)
{
	// Try to find the requested present mode in the available present modes
	auto const present_mode_it = std::ranges::find(available_present_modes, request_present_mode);
	if (present_mode_it == available_present_modes.end())
	{
		// If the requested present mode isn't found, then try to find a mode from the priority list
		auto const chosen_present_mode_it =
		    std::ranges::find_if(present_mode_priority_list,
		                         [&available_present_modes](vk::PresentModeKHR present_mode) { return std::ranges::find(available_present_modes, present_mode) != available_present_modes.end(); });

		// If nothing found, always default to FIFO
		vk::PresentModeKHR const chosen_present_mode = (chosen_present_mode_it != present_mode_priority_list.end()) ? *chosen_present_mode_it : vk::PresentModeKHR::eFifo;

		LOGW("(Swapchain) Present mode '{}' not supported. Selecting '{}'.", vk::to_string(request_present_mode), vk::to_string(chosen_present_mode));
		return chosen_present_mode;
	}
	else
	{
		LOGI("(Swapchain) Present mode selected: {}", to_string(request_present_mode));
		return request_present_mode;
	}
}

vk::SurfaceFormatKHR choose_surface_format(const vk::SurfaceFormatKHR               requested_surface_format,
                                           const std::vector<vk::SurfaceFormatKHR> &available_surface_formats,
                                           const std::vector<vk::SurfaceFormatKHR> &surface_format_priority_list)
{
	// Try to find the requested surface format in the available surface formats
	auto const surface_format_it = std::find(available_surface_formats.begin(), available_surface_formats.end(), requested_surface_format);

	// If the requested surface format isn't found, then try to request a format from the priority list
	if (surface_format_it == available_surface_formats.end())
	{
		auto const chosen_surface_format_it =
		    std::ranges::find_if(surface_format_priority_list,
		                         [&available_surface_formats](vk::SurfaceFormatKHR surface_format) { return std::ranges::find(available_surface_formats, surface_format) != available_surface_formats.end(); });

		// If nothing found, default to the first available format
		vk::SurfaceFormatKHR const &chosen_surface_format = (chosen_surface_format_it != surface_format_priority_list.end()) ? *chosen_surface_format_it : available_surface_formats[0];

		LOGW("(Swapchain) Surface format ({}) not supported. Selecting ({}).",
		     vk::to_string(requested_surface_format.format) + ", " + vk::to_string(requested_surface_format.colorSpace),
		     vk::to_string(chosen_surface_format.format) + ", " + vk::to_string(chosen_surface_format.colorSpace));
		return chosen_surface_format;
	}
	else
	{
		LOGI("(Swapchain) Surface format selected: {}",
		     vk::to_string(requested_surface_format.format) + ", " + vk::to_string(requested_surface_format.colorSpace));
		return requested_surface_format;
	}
}

vk::SurfaceTransformFlagBitsKHR choose_transform(vk::SurfaceTransformFlagBitsKHR request_transform,
                                                 vk::SurfaceTransformFlagsKHR    supported_transform,
                                                 vk::SurfaceTransformFlagBitsKHR current_transform)
{
	if (request_transform & supported_transform)
	{
		return request_transform;
	}

	LOGW("(Swapchain) Surface transform '{}' not supported. Selecting '{}'.", vk::to_string(request_transform), vk::to_string(current_transform));
	return current_transform;
}

vk::CompositeAlphaFlagBitsKHR choose_composite_alpha(vk::CompositeAlphaFlagBitsKHR request_composite_alpha,
                                                     vk::CompositeAlphaFlagsKHR    supported_composite_alpha)
{
	if (request_composite_alpha & supported_composite_alpha)
	{
		return request_composite_alpha;
	}

	static const std::vector<vk::CompositeAlphaFlagBitsKHR> composite_alpha_priority_list = {vk::CompositeAlphaFlagBitsKHR::eOpaque,
	                                                                                         vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
	                                                                                         vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
	                                                                                         vk::CompositeAlphaFlagBitsKHR::eInherit};

	auto const chosen_composite_alpha_it =
	    std::find_if(composite_alpha_priority_list.begin(),
	                 composite_alpha_priority_list.end(),
	                 [&supported_composite_alpha](vk::CompositeAlphaFlagBitsKHR composite_alpha) { return composite_alpha & supported_composite_alpha; });
	if (chosen_composite_alpha_it == composite_alpha_priority_list.end())
	{
		throw std::runtime_error("No compatible composite alpha found.");
	}
	else
	{
		LOGW("(Swapchain) Composite alpha '{}' not supported. Selecting '{}.", vk::to_string(request_composite_alpha), vk::to_string(*chosen_composite_alpha_it));
		return *chosen_composite_alpha_it;
	}
}

bool validate_format_feature(vk::ImageUsageFlagBits image_usage, vk::FormatFeatureFlags supported_features)
{
	return (image_usage != vk::ImageUsageFlagBits::eStorage) || (supported_features & vk::FormatFeatureFlagBits::eStorageImage);
}

std::set<vk::ImageUsageFlagBits> choose_image_usage(const std::set<vk::ImageUsageFlagBits> &requested_image_usage_flags,
                                                    vk::ImageUsageFlags                     supported_image_usage,
                                                    vk::FormatFeatureFlags                  supported_features)
{
	std::set<vk::ImageUsageFlagBits> validated_image_usage_flags;
	for (auto flag : requested_image_usage_flags)
	{
		if ((flag & supported_image_usage) && validate_format_feature(flag, supported_features))
		{
			validated_image_usage_flags.insert(flag);
		}
		else
		{
			LOGW("(Swapchain) Image usage ({}) requested but not supported.", vk::to_string(flag));
		}
	}

	if (validated_image_usage_flags.empty())
	{
		// Pick the first format from list of defaults, if supported
		static const std::vector<vk::ImageUsageFlagBits> image_usage_priority_list = {
		    vk::ImageUsageFlagBits::eColorAttachment, vk::ImageUsageFlagBits::eStorage, vk::ImageUsageFlagBits::eSampled, vk::ImageUsageFlagBits::eTransferDst};

		auto const priority_list_it =
		    std::ranges::find_if(image_usage_priority_list,
		                         [&supported_image_usage, &supported_features](auto const image_usage) { return ((image_usage & supported_image_usage) && validate_format_feature(image_usage, supported_features)); });
		if (priority_list_it != image_usage_priority_list.end())
		{
			validated_image_usage_flags.insert(*priority_list_it);
		}
	}

	if (validated_image_usage_flags.empty())
	{
		throw std::runtime_error("No compatible image usage found.");
	}
	else
	{
		// Log image usage flags used
		std::string usage_list;
		for (const vk::ImageUsageFlagBits image_usage : validated_image_usage_flags)
		{
			usage_list += to_string(image_usage) + " ";
		}
		LOGI("(Swapchain) Image usage flags: {}", usage_list);
	}

	return validated_image_usage_flags;
}

vk::ImageUsageFlags composite_image_flags(std::set<vk::ImageUsageFlagBits> &image_usage_flags)
{
	vk::ImageUsageFlags image_usage;
	for (const auto flag : image_usage_flags)
	{
		image_usage |= flag;
	}
	return image_usage;
}
}        // namespace


namespace xihe::backend
{
Swapchain::Swapchain(Swapchain &old_swapchain, const vk::Extent2D &extent):
    Swapchain{old_swapchain.device_,
              old_swapchain.surface_,
              old_swapchain.properties_.present_mode,
              old_swapchain.present_mode_priority_list_,
              old_swapchain.surface_format_priority_list_,
              extent,
              old_swapchain.properties_.image_count,
              old_swapchain.properties_.pre_transform,
              old_swapchain.image_usage_flags_,
              old_swapchain.get_handle()}
{}

Swapchain::Swapchain(Swapchain &old_swapchain, const uint32_t image_count):
    Swapchain{old_swapchain.device_,
              old_swapchain.surface_,
              old_swapchain.properties_.present_mode,
              old_swapchain.present_mode_priority_list_,
              old_swapchain.surface_format_priority_list_,
              old_swapchain.properties_.extent,
              image_count,
              old_swapchain.properties_.pre_transform,
              old_swapchain.image_usage_flags_,
              old_swapchain.get_handle()}
{}

Swapchain::Swapchain(Swapchain &old_swapchain, const std::set<vk::ImageUsageFlagBits> &image_usage_flags):
    Swapchain{old_swapchain.device_,
              old_swapchain.surface_,
              old_swapchain.properties_.present_mode,
              old_swapchain.present_mode_priority_list_,
              old_swapchain.surface_format_priority_list_,
              old_swapchain.properties_.extent,
              old_swapchain.properties_.image_count,
              old_swapchain.properties_.pre_transform,
              image_usage_flags,
              old_swapchain.get_handle()}
{}

Swapchain::Swapchain(Device                                  &device,
                     vk::SurfaceKHR                           surface,
                     const vk::PresentModeKHR                 present_mode,
                     const std::vector<vk::PresentModeKHR>   &present_mode_priority_list,
                     const std::vector<vk::SurfaceFormatKHR> &surface_format_priority_list,
                     const vk::Extent2D &extent, const uint32_t image_count,
                     const vk::SurfaceTransformFlagBitsKHR   transform,
                     const std::set<vk::ImageUsageFlagBits> &image_usage_flags,
                     vk::SwapchainKHR                        old_swapchain) :
    device_{device},
    surface_{surface}
{
	present_mode_priority_list_   = present_mode_priority_list;
	surface_format_priority_list_ = surface_format_priority_list;

	surface_formats_ = device_.get_gpu().get_handle().getSurfaceFormatsKHR(surface);
	LOGI("Surface supports the following surface formats:");
	for (auto &surface_format : surface_formats_)
	{
		LOGI("  \t{}", vk::to_string(surface_format.format) + ", " + vk::to_string(surface_format.colorSpace));
	}

	// Choose the best properties based on surface capabilities
	vk::SurfaceCapabilitiesKHR const surface_capabilities = device_.get_gpu().get_handle().getSurfaceCapabilitiesKHR(surface);

	vk::FormatProperties const format_properties = device_.get_gpu().get_handle().getFormatProperties(properties_.surface_format.format);
	image_usage_flags_ = choose_image_usage(image_usage_flags, surface_capabilities.supportedUsageFlags, format_properties.optimalTilingFeatures);

	properties_.image_count = clamp(image_count, surface_capabilities.minImageCount,
	                                surface_capabilities.maxImageCount ? surface_capabilities.maxImageCount : std::numeric_limits<uint32_t>::max());
	properties_.extent          = choose_extent(extent, surface_capabilities.minImageExtent, surface_capabilities.maxImageExtent, surface_capabilities.currentExtent);
	properties_.array_layers    = 1;
	properties_.surface_format  = choose_surface_format(properties_.surface_format, surface_formats_, surface_format_priority_list);
	properties_.image_usage     = composite_image_flags(image_usage_flags_);
	properties_.pre_transform   = choose_transform(transform, surface_capabilities.supportedTransforms, surface_capabilities.currentTransform);
	properties_.composite_alpha = choose_composite_alpha(vk::CompositeAlphaFlagBitsKHR::eInherit, surface_capabilities.supportedCompositeAlpha);

	properties_.old_swapchain = old_swapchain;
	properties_.present_mode  = present_mode;

	// Revalidate the present mode and surface format
	properties_.present_mode   = choose_present_mode(properties_.present_mode, present_modes_, present_mode_priority_list);
	properties_.surface_format = choose_surface_format(properties_.surface_format, surface_formats_, surface_format_priority_list);

	vk::SwapchainCreateInfoKHR const create_info({},
	                                             surface,
	                                             properties_.image_count,
	                                             properties_.surface_format.format,
	                                             properties_.surface_format.colorSpace,
	                                             properties_.extent,
	                                             properties_.array_layers,
	                                             properties_.image_usage,
	                                             {},
	                                             {},
	                                             properties_.pre_transform,
	                                             properties_.composite_alpha,
	                                             properties_.present_mode,
	                                             {},
	                                             properties_.old_swapchain);

	handle_ = device.get_handle().createSwapchainKHR(create_info);

	images_ = device.get_handle().getSwapchainImagesKHR(handle_);
}

Swapchain::~Swapchain()
{
	if (handle_)
	{
		device_.get_handle().destroySwapchainKHR(handle_);
	}
}

Device const & Swapchain::get_device() const
{
	return device_;
}

vk::SwapchainKHR Swapchain::get_handle() const
{
	return handle_;
}

std::pair<vk::Result, uint32_t> Swapchain::acquire_next_image(vk::Semaphore image_acquired_semaphore, vk::Fence fence) const
{
	vk::ResultValue<uint32_t> result = device_.get_handle().acquireNextImageKHR(handle_, std::numeric_limits<uint64_t>::max(), image_acquired_semaphore, fence);
	return std::make_pair(result.result, result.value);
}


vk::Format Swapchain::get_format() const
{
	return properties_.surface_format.format;
}

vk::ImageUsageFlags Swapchain::get_image_usage() const
{
	return properties_.image_usage;
}

vk::SurfaceTransformFlagBitsKHR Swapchain::get_transform() const
{
	return properties_.pre_transform;
}

vk::SurfaceKHR Swapchain::get_surface() const
{
	return surface_;
}

vk::PresentModeKHR Swapchain::get_present_mode() const
{
	return properties_.present_mode;
}

const vk::Extent2D & Swapchain::get_extent() const
{
	return properties_.extent;
}

const std::vector<vk::Image> & Swapchain::get_images() const
{
	return images_;
}
}        // namespace xihe::backend
