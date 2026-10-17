#include "vtex_parser.h"
#include <sdk/interfaces/filesystem.h>
#include <cstring>
#include <algorithm>
#include <span>
#include <utils/stb_image.h>

#define LZ4_DISABLE_DEPRECATE_WARNINGS
#include "lz4.h"

namespace vtex_parser {
	enum class vtex_format : uint8_t {
		UNKNOWN = 0,
		DXT1 = 1,
		DXT5 = 2,
		I8 = 3,
		RGBA8 = 4,
		PNG_RGBA8 = 16,
		BGRA8 = 28
	};

	enum class vtex_extra_data : uint32_t {
		UNKNOWN = 0,
		FALLBACK_BITS = 1,
		SHEET = 2,
		METADATA = 3,
		COMPRESSED_MIP_SIZE = 4,
		CUBEMAP_RADIANCE_SH = 5
	};

	struct vtex_header {
		uint16_t version;
		uint16_t flags;
		uint32_t reflectivity[4];
		uint16_t width;
		uint16_t height;
		uint16_t depth;
		vtex_format format;
		uint8_t mip_count;
		uint32_t picmip0_res;
		uint32_t extra_data_offset;
		uint32_t extra_data_count;
	};

	struct memstream {
		char* data;
		int pos;

		memstream(char* d) : data(d), pos(0) {}

		void seekg(int offset) {
			pos += offset;
		}

		template<typename T = char>
		memstream& read(T* v, int n) {
			std::memcpy(v, data + pos, n * sizeof(T));
			pos += sizeof(T) * n;
			return *this;
		}

		template<typename T>
		memstream& operator>>(T& val) {
			std::memcpy(&val, data + pos, sizeof(T));
			pos += sizeof(T);
			return *this;
		}
	};

	int mip_level_size(int size, uint32_t level) {
		size >>= (int)level;
		return std::max(size, 1);
	}

	int calculate_buffer_size_for_mip_level(uint32_t mip_level, uint32_t width, uint32_t height) {
		auto bytes_per_pixel = 4;
		auto width_ = mip_level_size(width, mip_level);
		auto height_ = mip_level_size(height, mip_level);
		return width_ * height_ * 1 * bytes_per_pixel;
	}

	int skip_mipmaps(int desired_mip_level, int num_mip_levels, int width, int height, int* compressed_mips) {
		if (num_mip_levels < 2)
			return 0;

		int size_ = 0;

		for (int j = num_mip_levels - 1; j > desired_mip_level; j--) {
			int size = calculate_buffer_size_for_mip_level(j, width, height);

			if (compressed_mips) {
				int compressed_size = compressed_mips[j];
				if (size > compressed_size) {
					size = compressed_size;
				}
			}

			size_ += size;
		}

		return size_;
	}

	vtex_data load(std::string_view filename, c_file_system* file_system) {
		vtex_data result;

		if (!file_system)
			return result;

		auto handle = file_system->open(filename.data(), "r", "GAME");
		if (!handle)
			return result;

		auto size = file_system->size(handle);
		std::vector<char> image(size);
		file_system->read(image.data(), size, handle);
		file_system->close(handle);

		if (image.empty())
			return result;

		int hdr_pos = 0;
		for (int i = 0; i < image.size(); i++) {
			if (*(uint32_t*)(image.data() + i) == 0x41544144) {
				hdr_pos = *(uint32_t*)(image.data() + i + 4) + i + 4;
				break;
			}
		}

		memstream s(image.data());
		s.pos = hdr_pos;

		vtex_header hdr;
		s >> hdr;

		std::vector<int> compressed_mips;
		auto actual_width = hdr.width;
		auto actual_height = hdr.height;
		bool is_compressed = false;
		uint32_t mips = 0;
		int extra_data_size = 0;

		if (hdr.extra_data_count > 0) {
			s.seekg(hdr.extra_data_offset - 8);

			for (auto i = 0; i < hdr.extra_data_count; i++) {
				vtex_extra_data type{};
				int offset = 0, size_data = 0;
				s >> type >> offset >> size_data;
				offset -= 8;

				extra_data_size += size_data;

				auto prev_offset = s.pos;
				s.pos += offset;

				if (type == vtex_extra_data::METADATA) {
					s.pos += 2;
					uint16_t nw, nh;
					s >> nw >> nh;
					if (nw > 0 && nh > 0 && actual_width >= nw && actual_height >= nh) {
						actual_width = nw;
						actual_height = nh;
					}
				} else if (type == vtex_extra_data::COMPRESSED_MIP_SIZE) {
					uint32_t int1, mips_offset;
					s >> int1 >> mips_offset >> mips;

					is_compressed = int1 == 1;

					compressed_mips.resize(mips);

					s.pos += mips_offset - 8;

					for (int mip = 0; mip < mips; mip++) {
						s >> compressed_mips[mip];
					}
				}

				s.pos = prev_offset;
			}
		}

		s.pos += extra_data_size;
		
		int* mips_ptr = compressed_mips.empty() ? nullptr : compressed_mips.data();
		s.pos += skip_mipmaps(0, mips, actual_width, actual_width, mips_ptr);

		auto input_data = s.pos + s.data;
		auto data_size = image.size() - s.pos;

		result.w = actual_width;
		result.h = actual_height;

		using enum vtex_format;
		switch (hdr.format) {
		case RGBA8:
		case BGRA8:
		{
			if (!is_compressed)
				return result;

			auto uncompressed_size = calculate_buffer_size_for_mip_level(0, result.w, result.h);

			s.seekg(8);

			auto raw_data = s.pos + s.data;

			auto compressed_size = compressed_mips[0];

			std::span<char> buffer(raw_data, compressed_size);

			auto span = std::as_const(buffer).subspan(0, compressed_size);

			s.read(span.data(), compressed_size);

			result.data.resize(uncompressed_size);

			LZ4_decompress_safe(span.data(), (char*)result.data.data(), compressed_size, uncompressed_size);

			if (hdr.format == vtex_format::BGRA8) {
				for (int i = 0; i < uncompressed_size; i += 4) {
					std::swap(result.data[i], result.data[i + 2]);
				}
			}

			break;
		}
		case PNG_RGBA8: {
			int x{}, y{}, comp{};

			stbi_set_flip_vertically_on_load(0);
			stbi_set_flip_vertically_on_load_thread(0);
			auto png = stbi_load_from_memory((stbi_uc*)input_data, data_size, &x, &y, &comp, 4);

			result.data.resize(x * y * 4);
			memcpy(result.data.data(), png, x * y * 4);

			stbi_image_free(png);
			break;
		}
		case DXT5:
		{
			break;
		}
		}

		return result;
	}
}
