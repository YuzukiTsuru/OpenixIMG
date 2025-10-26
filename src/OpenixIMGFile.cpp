/**
 * @file OpenixIMGFile.cpp
 * @brief Implementation of OpenixIMGFile class methods
 * @author YuzukiTsuru <gloomyghost@gloomyghost.com>
 */

#include <fstream>
#include <filesystem>
#include <cstring>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <optional>

#include "OpenixIMGWTY.hpp"
#include "OpenixIMGFile.hpp"

#include <algorithm>

using namespace OpenixIMG;
namespace fs = std::filesystem;

OpenixIMGFile::OpenixIMGFile() : encryptionEnabled_(true),
                                 verbose_(false),
                                 imageLoaded_(false),
                                 imageSize_(0),
                                 twofishKey_(32, 0) {
    initializeCrypto();
}

OpenixIMGFile::OpenixIMGFile(const std::string &imageFilePath) : encryptionEnabled_(true),
                                                                 verbose_(false),
                                                                 imageLoaded_(false),
                                                                 imageSize_(0),
                                                                 twofishKey_(32, 0) {
    initializeCrypto();
    loadImage(imageFilePath);
}

OpenixIMGFile::~OpenixIMGFile() = default;

void OpenixIMGFile::setEncryptionEnabled(const bool enabled) {
    encryptionEnabled_ = enabled;
}

void OpenixIMGFile::setVerbose(const bool verbose) {
    verbose_ = verbose;
}

bool OpenixIMGFile::loadImage(const std::string &imageFilePath) {
    try {
        // Check if file exists
        if (!fs::exists(imageFilePath)) {
            std::cerr << "Error: unable to open " << imageFilePath << "!" << std::endl;
            return false;
        }

        // Get file size
        imageSize_ = static_cast<ssize_t>(fs::file_size(imageFilePath));
        if (imageSize_ == 0) {
            std::cerr << "Error: Invalid file size 0" << std::endl;
            return false;
        }

        // Read entire image into memory
        imageData_.resize(imageSize_);
        std::ifstream inFile(imageFilePath, std::ios::binary);
        if (!inFile.is_open()) {
            std::cerr << "Error: unable to open " << imageFilePath << "!" << std::endl;
            return false;
        }

        inFile.read(reinterpret_cast<char *>(imageData_.data()), imageSize_);
        inFile.close();

        // Store file path
        imageFilePath_ = imageFilePath;

        // Parse image header
        imageHeader_ = *reinterpret_cast<ImageHeader *>(imageData_.data());

        // Check for encryption
        isEncrypted_ = (std::memcmp(imageHeader_.magic.data(), IMAGEWTY_MAGIC, IMAGEWTY_MAGIC_LEN) != 0);

        if (isEncrypted_ && encryptionEnabled_) {
            // Decrypt header
            rc6DecryptInPlace(imageData_.data(), 1024, headerContext_);
            // Update the imageHeader_ with decrypted data
            imageHeader_ = *reinterpret_cast<ImageHeader *>(imageData_.data());
        }

        // Decrypt file headers if needed
        uint32_t numFiles = 0;
        if (imageHeader_.header_version == 0x0300) {
            numFiles = imageHeader_.v3.num_files;
        } else {
            numFiles = imageHeader_.v1.num_files;
        }

        if (isEncrypted_ && encryptionEnabled_) {
            rc6DecryptInPlace(imageData_.data() + 1024, numFiles * 1024, fileHeadersContext_);
        }

        // Decrypt file contents if needed
        uint8_t *current = imageData_.data() + 1024 + numFiles * 1024;
        for (uint32_t i = 0; i < numFiles; ++i) {
            auto *fileHeader = reinterpret_cast<const FileHeader *>(imageData_.data() + 1024 + i * 1024);

            uint32_t storedLength = 0;

            if (imageHeader_.header_version == 0x0300) {
                storedLength = fileHeader->v3.stored_length;
            } else {
                storedLength = fileHeader->v1.stored_length;
            }

            // Decrypt with RC6 if needed
            if (isEncrypted_ && encryptionEnabled_) {
                current = static_cast<uint8_t *>(rc6DecryptInPlace(current, storedLength, fileContentContext_));
            }
        }

        // Get image metadata
        if (imageHeader_.header_version == 0x0300) {
            hardwareId_ = imageHeader_.v3.hardware_id;
            firmwareId_ = imageHeader_.v3.firmware_id;
            pid_ = imageHeader_.v3.pid;
            vid_ = imageHeader_.v3.vid;
        } else {
            hardwareId_ = imageHeader_.v1.hardware_id;
            firmwareId_ = imageHeader_.v1.firmware_id;
            pid_ = imageHeader_.v1.pid;
            vid_ = imageHeader_.v1.vid;
        }

        // Load file list
        loadFileList();

        // Mark image as loaded
        imageLoaded_ = true;

        if (verbose_) {
            std::cout << "Successfully loaded image: " << imageFilePath << " (size: " << imageSize_ << " bytes)" <<
                    std::endl;
            std::cout << "Found " << fileList_.size() << " files in image" << std::endl;
        }

        return true;
    } catch (const std::exception &e) {
        std::cerr << "Error loading image: " << e.what() << std::endl;
        imageLoaded_ = false;
        return false;
    }
}

std::string OpenixIMGFile::getImageFilePath() const {
    return imageFilePath_;
}

bool OpenixIMGFile::isImageLoaded() const {
    return imageLoaded_;
}

void OpenixIMGFile::freeImage() {
    // Clear image data to release memory
    imageData_.clear();
    imageData_.shrink_to_fit();

    // Clear file list
    fileList_.clear();
    fileList_.shrink_to_fit();

    // Reset state variables
    imageLoaded_ = false;
    imageSize_ = 0;

    // Reset metadata
    pid_ = 0;
    vid_ = 0;
    hardwareId_ = 0;
    firmwareId_ = 0;

    // Note: We keep the imageFilePath_ so that reloadImage can still work

    if (verbose_) {
        std::cout << "Image data freed successfully" << std::endl;
    }
}

bool OpenixIMGFile::reloadImage() {
    // Check if the new image file path is empty
    if (imageFilePath_.empty()) {
        std::cerr << "Error: No image file path provided" << std::endl;
        return false;
    }

    if (verbose_) {
        std::cout << "Reloading image with new path: " << imageFilePath_ << std::endl;
    }

    // Free current image data
    freeImage();

    // Load the new image
    return loadImage(imageFilePath_);
}

bool OpenixIMGFile::reloadImage(const std::string &newImageFilePath) {
    // Check if the new image file path is empty
    if (newImageFilePath.empty()) {
        std::cerr << "Error: No image file path provided" << std::endl;
        return false;
    }

    if (verbose_) {
        std::cout << "Reloading image with new path: " << newImageFilePath << std::endl;
    }

    // Free current image data
    freeImage();

    // Load the new image
    return loadImage(newImageFilePath);
}

uint32_t OpenixIMGFile::getPID() const {
    return pid_;
}

uint32_t OpenixIMGFile::getVID() const {
    return vid_;
}

uint32_t OpenixIMGFile::getHardwareId() const {
    return hardwareId_;
}

uint32_t OpenixIMGFile::getFirmwareId() const {
    return firmwareId_;
}

void OpenixIMGFile::initializeCrypto() {
    // Initialize RC6 context for header
    std::vector<uint8_t> headerKey(32, 0);
    headerKey[31] = 'i'; // Last byte is 'i'
    headerContext_.init(headerKey.data(), headerKey.size() * 8);

    // Initialize RC6 context for file headers
    std::vector<uint8_t> fileHeadersKey(32, 1);
    fileHeadersKey[31] = 'm'; // Last byte is 'm'
    fileHeadersContext_.init(fileHeadersKey.data(), fileHeadersKey.size() * 8);

    // Initialize RC6 context for file content
    std::vector<uint8_t> fileContentKey(32, 2);
    fileContentKey[31] = 'g'; // Last byte is 'g'
    fileContentContext_.init(fileContentKey.data(), fileContentKey.size() * 8);

    // Initialize TwoFish key for file content of non-fex files
    twofishKey_[0] = 5;
    twofishKey_[1] = 4;
    for (size_t i = 2; i < twofishKey_.size(); ++i) {
        twofishKey_[i] = twofishKey_[i - 2] + twofishKey_[i - 1];
    }

    // Initialize TwoFish context
    twofishContext_.initialize(twofishKey_, 256);
}

void OpenixIMGFile::loadFileList() {
    // Clear existing file list
    fileList_.clear();

    // Get number of files
    uint32_t numFiles = 0;
    if (imageHeader_.header_version == 0x0300) {
        numFiles = imageHeader_.v3.num_files;
    } else {
        numFiles = imageHeader_.v1.num_files;
    }

    // Parse each file header and store in fileList_
    for (uint32_t i = 0; i < numFiles; ++i) {
        auto *fileHeader = reinterpret_cast<const FileHeader *>(imageData_.data() + 1024 + i * 1024);

        FileInfo info;

        // Extract common information
        info.maintype = std::string(fileHeader->maintype.data(), IMAGEWTY_FHDR_MAINTYPE_LEN);
        info.subtype = std::string(fileHeader->subtype.data(), IMAGEWTY_FHDR_SUBTYPE_LEN);

        // Extract version-specific information
        if (imageHeader_.header_version == 0x0300) {
            info.filename = fileHeader->v3.filename.data();
            info.storedLength = fileHeader->v3.stored_length;
            info.originalLength = fileHeader->v3.original_length;
            info.offset = fileHeader->v3.offset;
        } else {
            info.filename = fileHeader->v1.filename.data();
            info.storedLength = fileHeader->v1.stored_length;
            info.originalLength = fileHeader->v1.original_length;
            info.offset = fileHeader->v1.offset;
        }

        // Clean up strings (remove trailing nulls and whitespace)
        info.filename.erase(std::find_if(
                                info.filename.rbegin(),
                                info.filename.rend(),
                                [](const unsigned char ch) {
                                    return !std::isspace(ch) && ch != '\0';
                                }).base(),
                            info.filename.end());
        info.maintype.erase(std::find_if(
                                info.maintype.rbegin(),
                                info.maintype.rend(),
                                [](const unsigned char ch) {
                                    return !std::isspace(ch) && ch != '\0';
                                }).base(),
                            info.maintype.end());
        info.subtype.erase(std::find_if(
                               info.subtype.rbegin(),
                               info.subtype.rend(),
                               [](const unsigned char ch) {
                                   return !std::isspace(ch) && ch != '\0';
                               }).base(),
                           info.subtype.end());

        fileList_.push_back(std::move(info));
    }
}

bool OpenixIMGFile::checkFileByFilename(const std::string &filename) const {
    try {
        // Check if an image is loaded
        if (!imageLoaded_) {
            std::cerr << "Error: no image file loaded!" << std::endl;
            return false;
        }

        // Search for the file by filename
        bool found = false;
        for (const auto &fileInfo: fileList_) {
            if (fileInfo.filename == filename) {
                found = true;
                if (verbose_) {
                    std::cout << "File found: " << filename << std::endl;
                }
                break;
            }
        }

        if (!found && verbose_) {
            std::cout << "File not found: " << filename << std::endl;
        }

        return found;
    } catch (const std::exception &e) {
        std::cerr << "Error checking file by filename: " << e.what() << std::endl;
        return false;
    }
}

bool OpenixIMGFile::checkFileBySubtype(const std::string &subtype) const {
    try {
        // Check if an image is loaded
        if (!imageLoaded_) {
            std::cerr << "Error: no image file loaded!" << std::endl;
            return false;
        }

        // Search for the file by subtype
        bool found = false;
        for (const auto &fileInfo: fileList_) {
            if (fileInfo.subtype == subtype) {
                found = true;
                if (verbose_) {
                    std::cout << "File with subtype found: " << subtype << std::endl;
                }
                break;
            }
        }

        if (!found && verbose_) {
            std::cout << "File with subtype not found: " << subtype << std::endl;
        }

        return found;
    } catch (const std::exception &e) {
        std::cerr << "Error checking file by subtype: " << e.what() << std::endl;
        return false;
    }
}

std::optional<FileHeader> OpenixIMGFile::getFileHeaderByFilename(const std::string &filename) const {
    try {
        // Check if an image is loaded
        if (!imageLoaded_) {
            std::cerr << "Error: no image file loaded!" << std::endl;
            return std::nullopt;
        }

        // Search for the file by filename
        for (size_t i = 0; i < fileList_.size(); ++i) {
            if (fileList_[i].filename == filename) {
                if (verbose_) {
                    std::cout << "File header found for: " << filename << std::endl;
                }
                // Return a copy of the file header
                auto *fileHeader = reinterpret_cast<const FileHeader *>(imageData_.data() + 1024 + i * 1024);
                return *fileHeader;
            }
        }

        if (verbose_) {
            std::cout << "File header not found for: " << filename << std::endl;
        }

        return std::nullopt;
    } catch (const std::exception &e) {
        std::cerr << "Error retrieving file header by filename: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::vector<FileHeader> OpenixIMGFile::getFileHeaderBySubtype(const std::string &subtype) const {
    try {
        // Check if an image is loaded
        if (!imageLoaded_) {
            std::cerr << "Error: no image file loaded!" << std::endl;
            return {};
        }

        // Search for files by subtype
        std::vector<FileHeader> results;
        for (size_t i = 0; i < fileList_.size(); ++i) {
            if (fileList_[i].subtype == subtype) {
                if (verbose_) {
                    std::cout << "File header found for subtype: " << subtype << " (file: " << fileList_[i].filename <<
                            ")" << std::endl;
                }
                // Add a copy of the file header to results
                auto *fileHeader = reinterpret_cast<const FileHeader *>(imageData_.data() + 1024 + i * 1024);
                results.push_back(*fileHeader);
            }
        }

        if (verbose_) {
            std::cout << "Found " << results.size() << " files with subtype: " << subtype << std::endl;
        }

        return results;
    } catch (const std::exception &e) {
        std::cerr << "Error retrieving file headers by subtype: " << e.what() << std::endl;
        return {};
    }
}

std::optional<std::vector<uint8_t> > OpenixIMGFile::getFileDataByFilename(const std::string &filename) const {
    try {
        // Check if an image is loaded
        if (!imageLoaded_) {
            std::cerr << "Error: no image file loaded!" << std::endl;
            return std::nullopt;
        }

        // Search for the file by filename
        for (const auto &fileInfo: fileList_) {
            if (fileInfo.filename == filename) {
                if (verbose_) {
                    std::cout << "Extracting data for: " << filename << " (size: " << fileInfo.originalLength <<
                            " bytes)" << std::endl;
                }
                // Extract file data
                std::vector<uint8_t> fileData(fileInfo.originalLength);
                std::memcpy(fileData.data(), imageData_.data() + fileInfo.offset, fileInfo.originalLength);
                return fileData;
            }
        }

        if (verbose_) {
            std::cout << "File data not found for: " << filename << std::endl;
        }

        return std::nullopt;
    } catch (const std::exception &e) {
        std::cerr << "Error retrieving file data by filename: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::vector<std::pair<std::string, std::vector<uint8_t> > > OpenixIMGFile::getFileDataBySubtype(
    const std::string &subtype) const {
    try {
        // Check if an image is loaded
        if (!imageLoaded_) {
            std::cerr << "Error: no image file loaded!" << std::endl;
            return {};
        }

        // Search for files by subtype
        std::vector<std::pair<std::string, std::vector<uint8_t> > > results;
        for (const auto &fileInfo: fileList_) {
            if (fileInfo.subtype == subtype) {
                if (verbose_) {
                    std::cout << "Extracting data for: " << fileInfo.filename << " (size: " << fileInfo.originalLength
                            << " bytes)" << std::endl;
                }
                // Extract file data
                std::vector<uint8_t> fileData(fileInfo.originalLength);
                std::memcpy(fileData.data(), imageData_.data() + fileInfo.offset, fileInfo.originalLength);
                results.emplace_back(fileInfo.filename, std::move(fileData));
            }
        }

        if (verbose_) {
            std::cout << "Found " << results.size() << " files with subtype: " << subtype << std::endl;
        }

        return results;
    } catch (const std::exception &e) {
        std::cerr << "Error retrieving file data by subtype: " << e.what() << std::endl;
        return {};
    }
}

void *OpenixIMGFile::rc6EncryptInPlace(void *data, const size_t length, const RC6 &context) {
    auto *current = static_cast<uint8_t *>(data);
    const auto numBlocks = length / 16;

    for (size_t i = 0; i < numBlocks; ++i) {
        context.encrypt(current);
        current += 16;
    }

    return current;
}

void *OpenixIMGFile::rc6DecryptInPlace(void *data, const size_t length, const RC6 &context) {
    auto *current = static_cast<uint8_t *>(data);
    const auto numBlocks = length / 16;

    for (size_t i = 0; i < numBlocks; ++i) {
        context.decrypt(current);
        current += 16;
    }

    return current;
}

void *OpenixIMGFile::twofishDecryptInPlace(void *data, const size_t length, const Twofish &context) {
    auto *current = static_cast<uint8_t *>(data);
    const auto numBlocks = length / 16;

    for (size_t i = 0; i < numBlocks; ++i) {
        // Convert to arrays for Twofish
        std::array<uint8_t, 16> inBlock{};
        std::array<uint8_t, 16> outBlock{};

        std::memcpy(inBlock.data(), current, 16);
        context.decrypt(inBlock, outBlock);
        std::memcpy(current, outBlock.data(), 16);

        current += 16;
    }

    return current;
}

const std::vector<uint8_t> &OpenixIMGFile::getImageData() const {
    return imageData_;
}

const ImageHeader &OpenixIMGFile::getImageHeader() const {
    return imageHeader_;
}

const std::vector<OpenixIMGFile::FileInfo> &OpenixIMGFile::getFileList() const {
    return fileList_;
}

bool OpenixIMGFile::isEncrypted() const {
    return isEncrypted_;
}
