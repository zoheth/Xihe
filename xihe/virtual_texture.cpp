#include "virtual_texture.h"

namespace xihe
{
bool TextureBlock::operator<(TextureBlock const &other) const
{
	if (new_mip_level == other.new_mip_level)
	{
		if (column == other.column)
		{
			return row < other.row;
		}
		return column < other.column;
	}
	return new_mip_level < other.new_mip_level;
}

void MemAllocInfo::get_allocation(PageInfo &page_memory_info, size_t page_index)
{
	// Check if we need to create a new memory sector:
	// - First allocation: no memory sectors yet
	// - Invalid sector: previous sector was released
	// - Full sector: no available space in current sector
	if (memory_sectors.empty() || memory_sectors.front().expired() || memory_sectors.front().lock()->available_offsets.empty())
	{
		// Create new memory sector and allocate from it
		page_memory_info.memory_sector = std::make_shared<MemSector>(*this);
		page_memory_info.offset        = *(page_memory_info.memory_sector->available_offsets.begin());

		page_memory_info.memory_sector->available_offsets.erase(page_memory_info.offset);
		page_memory_info.memory_sector->virtual_page_indices.insert(page_index);

		memory_sectors.push_front(page_memory_info.memory_sector);
	}
	else
	{
		// Reuse existing memory sector
		const auto ptr = memory_sectors.front().lock();

		page_memory_info.memory_sector = ptr;
		page_memory_info.offset        = *(page_memory_info.memory_sector->available_offsets.begin());

		page_memory_info.memory_sector->available_offsets.erase(page_memory_info.offset);
		page_memory_info.memory_sector->virtual_page_indices.insert(page_index);
	}
}

uint32_t MemAllocInfo::get_size() const
{
	return static_cast<uint32_t>(memory_sectors.size());
}

std::list<std::weak_ptr<MemSector>> &MemAllocInfo::get_memory_sectors()
{
	return memory_sectors;
}

MemSector::MemSector(MemAllocInfo &mem_alloc_info) :
    MemAllocInfo(mem_alloc_info)
{
	vk::MemoryAllocateInfo memory_allocate_info;
	memory_allocate_info.allocationSize  = page_size * pages_per_allocation;
	memory_allocate_info.memoryTypeIndex = memory_type_index;

	memory = device.allocateMemory(memory_allocate_info, nullptr);

	for (size_t i = 0; i < pages_per_allocation; ++i)
	{
		available_offsets.insert(static_cast<uint32_t>(i * page_size));
	}
}

MemSector::~MemSector()
{
	device.waitIdle();
	device.freeMemory(memory, nullptr);
}

bool MemSectorCompare::operator()(const std::weak_ptr<MemSector> &left, const std::weak_ptr<MemSector> &right)
{
	if (left.expired())
	{
		return false;
	}
	else if (right.expired())
	{
		return true;
	}
	return left.lock()->available_offsets.size() > right.lock()->available_offsets.size();
}

void VirtualTexture::create_sparse_texture_image(backend::Device &device)
{
	{
		backend::ImageBuilder image_builder(width, height);

		image_builder.with_usage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled);
		image_builder.with_flags(vk::ImageCreateFlagBits::eSparseBinding | vk::ImageCreateFlagBits::eSparseResidency);
		image_builder.with_format(raw_data_image->get_format());

		base_mip_level = 0U;
		mip_levels     = 5U;

		image_builder.with_mip_levels(mip_levels);
		image_builder.with_sample_count(vk::SampleCountFlagBits::e1);
		image_builder.with_tiling(vk::ImageTiling::eOptimal);
		image_builder.with_sharing_mode(vk::SharingMode::eExclusive);

		texture_image = image_builder.build_unique(device);
	}

	std::vector<vk::SparseImageMemoryRequirements> sparse_image_memory_requirements;
	vk::MemoryRequirements                         memory_requirements;

	uint32_t sparse_image_requirement_count = 0U;

	device.get_handle().getImageSparseMemoryRequirements(texture_image->get_handle(), &sparse_image_requirement_count, sparse_image_memory_requirements.data());
	sparse_image_memory_requirements.resize(sparse_image_requirement_count);
	device.get_handle().getImageSparseMemoryRequirements(texture_image->get_handle(), &sparse_image_requirement_count, sparse_image_memory_requirements.data());

	// device.get_handle().getBufferMemoryRequirements(texture_image->get_handle(), &memory_requirements);
}

CalculateMipLevelData::CalculateMipLevelData(const glm::mat4 &mvp_transform, const vk::Extent2D &texture_base_dim, const vk::Extent2D &screen_base_dim, uint32_t vertical_num_blocks, uint32_t horizontal_num_blocks, uint8_t mip_levels) :
    mesh(vertical_num_blocks + 1U),
    vertical_num_blocks(vertical_num_blocks),
    horizontal_num_blocks(horizontal_num_blocks),
    mip_levels(mip_levels),
    ax_vertical(horizontal_num_blocks + 1U),
    ax_horizontal(vertical_num_blocks + 1U),
    mvp_transform(mvp_transform),
    texture_base_dim(texture_base_dim),
    screen_base_dim(screen_base_dim)
{
	for (auto &row : mesh)
	{
		row.resize(horizontal_num_blocks + 1U);
	}
}
}        // namespace xihe
