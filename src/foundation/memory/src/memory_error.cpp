#include <foundation/memory/include/memory_error.hpp>

#include <memory>
#include <string_view>

namespace opus3d::foundation::error_domains
{
	static size_t paste_error_string(std::span<char> strBuffer, std::string_view message)
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

	size_t memory_error_formatter(uint32_t code, std::span<char> strBuffer)
	{
		switch(static_cast<memory::MemoryErrorCode>(code))
		{
			case memory::MemoryErrorCode::OutOfMemory:
			{
				return paste_error_string(strBuffer, "Out of Memory!");
			}
			case memory::MemoryErrorCode::AllocatorNoResize:
			{
				return paste_error_string(strBuffer, "Allocator lacks resize fptr!");
			}
			default:
			{
				return paste_error_string(strBuffer, "Unknown Error!");
			}
		}
	}

} // namespace opus3d::foundation::error_domains