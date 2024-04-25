#pragma once

#include "backend/bu"

namespace xihe::backend
{
struct ResourceInfo
{
	bool dirty{false};
	const backend::Buffer *buffer{nullptr};
};
}
