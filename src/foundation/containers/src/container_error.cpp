#include <foundation/containers/include/container_error.hpp>

#include <string_view>

namespace opus3d::foundation::error_domains
{
	static size_t paste_error_string(std::span<char> strBuffer, std::string_view message) noexcept
	{
		if(strBuffer.empty())
		{
			return 0;
		}

		// Use n-1 to ensure we always have a slot for the \0 we will write.
		const size_t copyCount = std::min(message.size(), strBuffer.size() - 1);

		auto it = std::copy_n(message.data(), copyCount, strBuffer.begin());
		*it	= '\0';

		return copyCount + 1;
	}

	size_t container_error_formatter(uint32_t code, std::span<char> strBuffer) noexcept
	{
		switch(static_cast<ContainerErrorCode>(code))
		{
			case ContainerErrorCode::ContainerFull:
			{
				return paste_error_string(strBuffer, "Container full!");
			}
			default:
			{
				return paste_error_string(strBuffer, "Unknown Error!");
			}
		}
	}
}