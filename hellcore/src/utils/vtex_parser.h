#pragma once

#include <vector>
#include <cstdint>
#include <string_view>

class c_file_system;

namespace vtex_parser {
	struct vtex_data {
		std::vector<uint8_t> data;
		int w = 0;
		int h = 0;
	};

	vtex_data load(std::string_view filename, c_file_system* file_system);
}
