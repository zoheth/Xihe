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
