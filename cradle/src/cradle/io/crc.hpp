#ifndef CRADLE_IO_CRC_HPP
#define CRADLE_IO_CRC_HPP

#include <cradle/common.hpp>

namespace cradle {

uint32_t compute_crc32(uint32_t crc, void const* data, size_t size);

}

#endif
