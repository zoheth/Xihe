#pragma once
#include <functional>
#include <memory>

#include "backend/image.h"

namespace xihe
{
namespace backend
{
class Device;
}

namespace rendering
{
class RenderTarget
{
public:
	using CreateFunc = std::function<std::unique_ptr<RenderTarget>(backend::Image &&)>;


};
}
}
