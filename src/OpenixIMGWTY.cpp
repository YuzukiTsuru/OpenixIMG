/**
 * @file OpenixIMGWTY.cpp
 * @brief Implementation of OpenixIMGWTY format structures
 * @author YuzukiTsuru <gloomyghost@gloomyghost.com>
 */

#include <cstring>

#include "OpenixIMGWTY.hpp"

using namespace OpenixIMG;

ImageHeader::ImageHeader() : v1() {
    std::memcpy(magic.data(), IMAGEWTY_MAGIC, IMAGEWTY_MAGIC_LEN);
    header_version = 0;
    header_size = 0;
    ram_base = 0;
    version = 0;
    image_size = 0;
    image_header_size = 0;
}

void ImageHeader::initialize(const uint32_t _version, const uint32_t pid, const uint32_t vid,
                             const uint32_t hardware_id, const uint32_t firmware_id, const uint32_t num_files) {
    std::memcpy(magic.data(), IMAGEWTY_MAGIC, IMAGEWTY_MAGIC_LEN);
    header_version = 0x0100; // Using version 1 as default
    header_size = 0x50; // Should be 0x60 for version 0x0300
    ram_base = 0x04D00000;
    version = _version;
    image_size = 0; // Will be filled later
    image_header_size = 1024;

    // Initialize version 1 specific fields
    v1.pid = pid;
    v1.vid = vid;
    v1.hardware_id = hardware_id;
    v1.firmware_id = firmware_id;
    v1.val1 = 1;
    v1.val1024 = 1024;
    v1.num_files = num_files;
    v1.val1024_2 = 1024;
    v1.val0 = 0;
    v1.val0_2 = 0;
    v1.val0_3 = 0;
    v1.val0_4 = 0;
}

FileHeader::FileHeader() : maintype(), subtype(), v1() {
    filename_len = IMAGEWTY_FHDR_FILENAME_LEN;
    total_header_size = IMAGEWTY_FILEHDR_LEN;
}

void FileHeader::initialize(const std::string &filename, const std::string &maintype_, const std::string &subtype_,
                            const uint32_t size, const uint32_t offset) {
    filename_len = IMAGEWTY_FHDR_FILENAME_LEN;
    total_header_size = IMAGEWTY_FILEHDR_LEN;

    // Copy maintype and subtype (truncating if necessary)
    std::strncpy(this->maintype.data(), maintype_.c_str(), IMAGEWTY_FHDR_MAINTYPE_LEN);
    std::strncpy(this->subtype.data(), subtype_.c_str(), IMAGEWTY_FHDR_SUBTYPE_LEN);

    // Initialize version 1 specific fields
    std::strncpy(v1.filename.data(), filename.c_str(), IMAGEWTY_FHDR_FILENAME_LEN);
    v1.offset = offset;
    v1.stored_length = size;
    v1.original_length = size;

    // Round up stored_length to next 512-byte boundary
    if (v1.stored_length & 0x1FF) {
        v1.stored_length &= ~0x1FF;
        v1.stored_length += 0x200;
    }

    v1.unknown_3 = 0;
    v1.unknown = 0;
}
