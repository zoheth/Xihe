#pragma once

namespace xihe
{
template <class T>
uint32_t to_u32(T value)
{
	static_assert(std::is_arithmetic_v<T>, "T must be numeric");

	if (static_cast<uintmax_t>(value) > static_cast<uintmax_t>(std::numeric_limits<uint32_t>::max()))
	{
		throw std::runtime_error("to_u32() failed, value is too big to be converted to uint32_t");
	}

	return static_cast<uint32_t>(value);
}

template <typename T>
std::vector<uint8_t> to_bytes(const T &value)
{
	return std::vector<uint8_t>{reinterpret_cast<const uint8_t *>(&value),
	                            reinterpret_cast<const uint8_t *>(&value) + sizeof(T)};
}

}        // namespace xihe