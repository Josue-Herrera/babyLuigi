#pragma once

#include <fstream>
#include <string>

namespace cosmos {
inline namespace v1 {

	inline auto write_to_file(std::string const& filename, std::string const& contents) noexcept -> bool
	{
		try {
			std::ofstream ofs(filename, std::ios::out | std::ios::binary);
			if (ofs.is_open()) {
				ofs.write(contents.data(), static_cast<long>(contents.size()));
				ofs.close();
			}
			return ofs.is_open();
		}
		catch (std::exception const& e) {
			return false;
		}
	}
}
}