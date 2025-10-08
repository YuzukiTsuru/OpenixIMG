/**
 * @file OpenixPacker.cpp
 * @brief Implementation of OpenixPacker class methods
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
#include "OpenixPacker.hpp"
#include "OpenixCFG.hpp"

using namespace OpenixIMG;
namespace fs = std::filesystem;

OpenixPacker::OpenixPacker() : encryptionEnabled_(true),
                               outputFormat_(OutputFormat::UNIMG),
                               verbose_(false),
                               imageLoaded_(false),
                               imageSize_(0),
                               twofishKey_(32, 0) {
    initializeCrypto();
}

OpenixPacker::OpenixPacker(const std::string &imageFilePath) : encryptionEnabled_(true),
                                                               outputFormat_(OutputFormat::UNIMG),
                                                               verbose_(false),
                                                               imageLoaded_(false),
                                                               imageSize_(0),
                                                               twofishKey_(32, 0) {
    initializeCrypto();
    loadImage(imageFilePath);
}

OpenixPacker::~OpenixPacker() = default;

void OpenixPacker::setEncryptionEnabled(const bool enabled) {
    encryptionEnabled_ = enabled;
}

void OpenixPacker::setOutputFormat(const OutputFormat format) {
    outputFormat_ = format;
}

void OpenixPacker::setVerbose(const bool verbose) {
    verbose_ = verbose;
}

bool OpenixPacker::loadImage(const std::string &imageFilePath) {
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

        // Mark image as loaded
        imageLoaded_ = true;

        if (verbose_) {
            std::cout << "Successfully loaded image: " << imageFilePath << " (size: " << imageSize_ << " bytes)" <<
                    std::endl;
        }

        return true;
    } catch (const std::exception &e) {
        std::cerr << "Error loading image: " << e.what() << std::endl;
        imageLoaded_ = false;
        return false;
    }
}

std::string OpenixPacker::getImageFilePath() const {
    return imageFilePath_;
}

bool OpenixPacker::isImageLoaded() const {
    return imageLoaded_;
}

void OpenixPacker::freeImage() {
    // Clear image data to release memory
    imageData_.clear();
    imageData_.shrink_to_fit();
    
    // Reset state variables
    imageLoaded_ = false;
    imageSize_ = 0;
    
    // Note: We keep the imageFilePath_ so that reloadImage can still work
    
    if (verbose_) {
        std::cout << "Image data freed successfully" << std::endl;
    }
}

bool OpenixPacker::reloadImage() {
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

bool OpenixPacker::reloadImage(const std::string &newImageFilePath) {
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

void OpenixPacker::initializeCrypto() {
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

bool OpenixPacker::packImage(const std::string &inputDir, const std::string &outputFile) {
    // TODO
    return true;
}

bool OpenixPacker::decryptImage(const std::string &outputFile) const {
    try {
        // Check if an image is loaded
        if (!imageLoaded_) {
            std::cerr << "Error: no image file loaded!" << std::endl;
            return false;
        }

        // Create a copy of the image data to work with
        std::vector<uint8_t> decryptedImageData = imageData_;

        // If the image is encrypted, we need to decrypt it again for output
        if (isEncrypted_ && encryptionEnabled_) {
            // Re-encrypt header for output (since we need to write the decrypted version)
            // Make a copy of the encrypted header
            std::vector<uint8_t> encryptedHeader(1024);
            std::ifstream inFile(imageFilePath_, std::ios::binary);
            if (!inFile.is_open()) {
                std::cerr << "Error: unable to re-open " << imageFilePath_ << "!" << std::endl;
                return false;
            }
            inFile.read(reinterpret_cast<char *>(encryptedHeader.data()), 1024);
            inFile.close();

            // Use the encrypted header for the output
            std::memcpy(decryptedImageData.data(), encryptedHeader.data(), 1024);

            // Decrypt the header in our copy
            rc6DecryptInPlace(decryptedImageData.data(), 1024, headerContext_);

            // Get number of files
            uint32_t numFiles = 0;
            auto *header = reinterpret_cast<ImageHeader *>(decryptedImageData.data());
            if (header->header_version == 0x0300) {
                numFiles = header->v3.num_files;
            } else {
                numFiles = header->v1.num_files;
            }

            // Re-encrypt file headers for output
            std::vector<uint8_t> encryptedFileHeaders(numFiles * 1024);
            inFile.open(imageFilePath_, std::ios::binary);
            if (!inFile.is_open()) {
                std::cerr << "Error: unable to re-open " << imageFilePath_ << "!" << std::endl;
                return false;
            }
            inFile.seekg(1024);
            inFile.read(reinterpret_cast<char *>(encryptedFileHeaders.data()), numFiles * 1024);
            inFile.close();

            // Use the encrypted file headers for the output
            std::memcpy(decryptedImageData.data() + 1024, encryptedFileHeaders.data(), numFiles * 1024);

            // Decrypt the file headers in our copy
            rc6DecryptInPlace(decryptedImageData.data() + 1024, numFiles * 1024, fileHeadersContext_);

            // Re-encrypt file contents for output
            std::vector<uint8_t> encryptedFileContents(imageSize_ - 1024 - numFiles * 1024);
            inFile.open(imageFilePath_, std::ios::binary);
            if (!inFile.is_open()) {
                std::cerr << "Error: unable to re-open " << imageFilePath_ << "!" << std::endl;
                return false;
            }
            inFile.seekg(1024 + numFiles * 1024);
            inFile.read(reinterpret_cast<char *>(encryptedFileContents.data()), encryptedFileContents.size());
            inFile.close();

            // Use the encrypted file contents for the output
            std::memcpy(decryptedImageData.data() + 1024 + numFiles * 1024,
                        encryptedFileContents.data(), encryptedFileContents.size());

            // Decrypt the file contents in our copy
            uint8_t *current = decryptedImageData.data() + 1024 + numFiles * 1024;
            for (uint32_t i = 0; i < numFiles; ++i) {
                auto *fileHeader = reinterpret_cast<FileHeader *>(decryptedImageData.data() + 1024 + i * 1024);

                uint32_t storedLength = 0, originalLength = 0;
                const char *filename = nullptr;

                if (header->header_version == 0x0300) {
                    storedLength = fileHeader->v3.stored_length;
                    originalLength = fileHeader->v3.original_length;
                    filename = fileHeader->v3.filename.data();
                } else {
                    storedLength = fileHeader->v1.stored_length;
                    originalLength = fileHeader->v1.original_length;
                    filename = fileHeader->v1.filename.data();
                }
                if (verbose_) {
                    std::cout << "Found: " << filename << " (" << originalLength << ", " << storedLength << ")" <<
                            std::endl;
                }
                // Decrypt with RC6
                current = static_cast<uint8_t *>(rc6DecryptInPlace(current, storedLength, fileContentContext_));
            }
        }

        // Write decrypted image to output file
        std::ofstream outFile(outputFile, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Error: unable to open output file " << outputFile << "!" << std::endl;
            return false;
        }

        outFile.write(reinterpret_cast<const char *>(decryptedImageData.data()), imageSize_);
        outFile.close();

        if (verbose_) {
            std::cout << "Successfully decrypted image to " << outputFile << std::endl;
        }

        return true;
    } catch (const std::exception &e) {
        std::cerr << "Error decrypting image: " << e.what() << std::endl;
        return false;
    }
}

bool OpenixPacker::unpackImage(const std::string &outputDir) const {
    try {
        // Check if an image is loaded
        if (!imageLoaded_) {
            std::cerr << "Error: no image file loaded!" << std::endl;
            return false;
        }

        // Recreate output directory if it exists
        if (fs::exists(outputDir)) {
            fs::remove_all(outputDir);
        }

        if (!createDirectoryRecursive(outputDir)) {
            std::cerr << "Error: unable to create output directory " << outputDir << "!" << std::endl;
            return false;
        }

        // Get header information
        uint32_t numFiles = 0;
        uint32_t pid = 0, vid = 0, hardwareId = 0, firmwareId = 0;

        if (imageHeader_.header_version == 0x0300) {
            numFiles = imageHeader_.v3.num_files;
            hardwareId = imageHeader_.v3.hardware_id;
            firmwareId = imageHeader_.v3.firmware_id;
            pid = imageHeader_.v3.pid;
            vid = imageHeader_.v3.vid;
        } else {
            numFiles = imageHeader_.v1.num_files;
            hardwareId = imageHeader_.v1.hardware_id;
            firmwareId = imageHeader_.v1.firmware_id;
            pid = imageHeader_.v1.pid;
            vid = imageHeader_.v1.vid;
        }

        // Create OpenixCFG instance to build the configuration
        OpenixCFG cfg;

        // Create DIR_DEF group
        auto dirDefGroup = std::make_shared<Group>("DIR_DEF");
        auto inputDirVar = std::make_shared<Variable>("INPUT_DIR", ValueType::STRING);
        inputDirVar->setString("../");
        dirDefGroup->addVariable(inputDirVar);

        // Create FILELIST group
        auto fileListGroup = std::make_shared<Group>("FILELIST");

        // Process each file and add to FILELIST group
        for (uint32_t i = 0; i < numFiles; ++i) {
            auto *fileHeader = reinterpret_cast<const FileHeader *>(imageData_.data() + 1024 + i * 1024);

            uint32_t storedLength = 0, originalLength = 0;
            const char *filename = nullptr;
            std::string contFilename = {};
            uint32_t offset = 0;

            if (imageHeader_.header_version == 0x0300) {
                storedLength = fileHeader->v3.stored_length;
                originalLength = fileHeader->v3.original_length;
                filename = fileHeader->v3.filename.data();
                offset = fileHeader->v3.offset;
            } else {
                storedLength = fileHeader->v1.stored_length;
                originalLength = fileHeader->v1.original_length;
                filename = fileHeader->v1.filename.data();
                offset = fileHeader->v1.offset;
            }

            // Get maintype and subtype (explicitly specify lengths to avoid overflow)
            std::string maintype(fileHeader->maintype.data(), IMAGEWTY_FHDR_MAINTYPE_LEN);
            std::string subtype(fileHeader->subtype.data(), IMAGEWTY_FHDR_SUBTYPE_LEN);

            if (outputFormat_ == OutputFormat::UNIMG) {
                // UNIMG format output
                if (verbose_) {
                    std::cout << "Extracting: " << maintype << " " << subtype << " (" << originalLength << ", " <<
                            storedLength << ")" << std::endl;
                }

                // Create header and content filenames
                std::string hdrFilename = maintype + std::string("_").append(subtype).append(".hdr");
                contFilename = maintype + std::string("_").append(subtype);

                // Write header file
                std::string hdrPath = outputDir + std::string("/").append(hdrFilename);
                if (std::ofstream hdrFile(hdrPath, std::ios::binary); hdrFile.is_open()) {
                    hdrFile.write(reinterpret_cast<const char *>(fileHeader), fileHeader->total_header_size);
                    hdrFile.close();
                }

                // Write content file
                std::string contPath = outputDir + std::string("/").append(contFilename);
                if (std::ofstream contFile(contPath, std::ios::binary); contFile.is_open()) {
                    contFile.write(reinterpret_cast<const char *>(imageData_.data() + offset), originalLength);
                    contFile.close();
                }
            } else if (outputFormat_ == OutputFormat::IMGREPACKER) {
                // IMGREPACKER format output
                if (verbose_) {
                    std::cout << "Extracting " << filename << std::endl;
                }

                // Create directory structure if needed
                std::string filePath = outputDir + "/" + filename;
                if (size_t lastSlashPos = filePath.find_last_of("/\\"); lastSlashPos != std::string::npos) {
                    std::string dirPath = filePath.substr(0, lastSlashPos);
                    createDirectoryRecursive(dirPath);
                }

                // Write file content
                if (std::ofstream outFile(filePath, std::ios::binary); outFile.is_open()) {
                    outFile.write(reinterpret_cast<const char *>(imageData_.data() + offset), originalLength);
                    outFile.close();
                }

                // Prepare filename for config
                contFilename = filename;
                if (contFilename[0] == '/') {
                    contFilename = contFilename.substr(1);
                }
            }

            // Create list item for FILELIST
            auto listItem = std::make_shared<Variable>("", ValueType::LIST_ITEM);

            // Add filename to list item
            auto filenameVar = std::make_shared<Variable>("filename", ValueType::STRING);
            filenameVar->setString(contFilename);
            listItem->addItem(filenameVar);

            // Add maintype to list item
            auto maintypeVar = std::make_shared<Variable>("maintype", ValueType::STRING);
            maintypeVar->setString(maintype);
            listItem->addItem(maintypeVar);

            // Add subtype to list item
            auto subtypeVar = std::make_shared<Variable>("subtype", ValueType::STRING);
            subtypeVar->setString(subtype);
            listItem->addItem(subtypeVar);

            // Add list item to FILELIST group
            fileListGroup->addVariable(listItem);
        }

        // Create IMAGE_CFG group
        auto imageCfgGroup = std::make_shared<Group>("IMAGE_CFG");

        // Add version to IMAGE_CFG
        auto versionVar = std::make_shared<Variable>("version", ValueType::NUMBER);
        versionVar->setNumber(imageHeader_.version);
        imageCfgGroup->addVariable(versionVar);

        // Add pid to IMAGE_CFG
        auto pidVar = std::make_shared<Variable>("pid", ValueType::NUMBER);
        pidVar->setNumber(pid);
        imageCfgGroup->addVariable(pidVar);

        // Add vid to IMAGE_CFG
        auto vidVar = std::make_shared<Variable>("vid", ValueType::NUMBER);
        vidVar->setNumber(vid);
        imageCfgGroup->addVariable(vidVar);

        // Add hardwareid to IMAGE_CFG
        auto hardwareidVar = std::make_shared<Variable>("hardwareid", ValueType::NUMBER);
        hardwareidVar->setNumber(hardwareId);
        imageCfgGroup->addVariable(hardwareidVar);

        // Add firmwareid to IMAGE_CFG
        auto firmwareidVar = std::make_shared<Variable>("firmwareid", ValueType::NUMBER);
        firmwareidVar->setNumber(firmwareId);
        imageCfgGroup->addVariable(firmwareidVar);

        // Add imagename to IMAGE_CFG
        auto imagenameVar = std::make_shared<Variable>("imagename", ValueType::REFERENCE);
        imagenameVar->setReference(imageFilePath_);
        imageCfgGroup->addVariable(imagenameVar);

        // Add filelist to IMAGE_CFG
        auto filelistVar = std::make_shared<Variable>("filelist", ValueType::REFERENCE);
        filelistVar->setReference("FILELIST");
        imageCfgGroup->addVariable(filelistVar);

        // Add encrypt to IMAGE_CFG
        auto encryptVar = std::make_shared<Variable>("encrypt", ValueType::REFERENCE);
        if (isEncrypted_)
            encryptVar->setReference("1");
        else
            encryptVar->setReference("0");
        imageCfgGroup->addVariable(encryptVar);

        // Add groups to OpenixCFG instance using the addGroup method
        cfg.addGroup(dirDefGroup);
        cfg.addGroup(fileListGroup);
        cfg.addGroup(imageCfgGroup);

        // Open image.cfg for writing
        std::string configPath = outputDir + "/image.cfg";
        std::ofstream configFile(configPath);
        if (!configFile.is_open()) {
            std::cerr << "Error: unable to create configuration file " << configPath << "!" << std::endl;
            return false;
        }

        // Write header information to config file
        time_t now = time(nullptr);
        tm *timeinfo = localtime(&now);
        char timeStr[256];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);

        configFile << ";/**************************************************************************/\n";
        configFile << "; " << timeStr << "\n";
        configFile << "; generated by OpenixIMG\n";
        configFile << "; " << imageFilePath_ << "\n";
        configFile << ";/**************************************************************************/\n";

        // Use dumpToString() to get configuration content and write to file
        std::string configContent = cfg.dumpToString();
        configFile << configContent;

        configFile.close();

        if (verbose_) {
            std::cout << "Successfully unpacked image to " << outputDir << std::endl;
        }

        return true;
    } catch (const std::exception &e) {
        std::cerr << "Error unpacking image: " << e.what() << std::endl;
        return false;
    }
}

bool OpenixPacker::checkFileByFilename(const std::string &filename) const {
    try {
        // Check if an image is loaded
        if (!imageLoaded_) {
            std::cerr << "Error: no image file loaded!" << std::endl;
            return false;
        }

        // Search for the file by filename
        bool found = false;
        uint32_t numFiles = 0;

        if (imageHeader_.header_version == 0x0300) {
            numFiles = imageHeader_.v3.num_files;
        } else {
            numFiles = imageHeader_.v1.num_files;
        }

        for (uint32_t i = 0; i < numFiles; ++i) {
            auto *fileHeader = reinterpret_cast<const FileHeader *>(imageData_.data() + 1024 + i * 1024);
            const char *currentFilename = nullptr;

            if (imageHeader_.header_version == 0x0300) {
                currentFilename = fileHeader->v3.filename.data();
            } else {
                currentFilename = fileHeader->v1.filename.data();
            }

            if (currentFilename && std::strcmp(currentFilename, filename.c_str()) == 0) {
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
        std::cerr << "Error checking file: " << e.what() << std::endl;
        return false;
    }
}

std::optional<FileHeader> OpenixPacker::getFileHeaderByFilename(const std::string &filename) const {
    try {
        // Check if an image is loaded
        if (!imageLoaded_) {
            std::cerr << "Error: no image file loaded!" << std::endl;
            return std::nullopt;
        }

        // Search for the file by filename
        uint32_t numFiles = 0;

        if (imageHeader_.header_version == 0x0300) {
            numFiles = imageHeader_.v3.num_files;
        } else {
            numFiles = imageHeader_.v1.num_files;
        }

        for (uint32_t i = 0; i < numFiles; ++i) {
            auto *fileHeader = reinterpret_cast<const FileHeader *>(imageData_.data() + 1024 + i * 1024);
            const char *currentFilename = nullptr;

            if (imageHeader_.header_version == 0x0300) {
                currentFilename = fileHeader->v3.filename.data();
            } else {
                currentFilename = fileHeader->v1.filename.data();
            }

            if (currentFilename && std::strcmp(currentFilename, filename.c_str()) == 0) {
                if (verbose_) {
                    std::cout << "File header found for: " << filename << std::endl;
                }
                // Return a copy of the file header
                FileHeader result;
                std::memcpy(&result, fileHeader, sizeof(FileHeader));
                return result;
            }
        }

        if (verbose_) {
            std::cout << "File header not found for: " << filename << std::endl;
        }

        // File not found
        return std::nullopt;
    } catch (const std::exception &e) {
        std::cerr << "Error retrieving file header: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<std::vector<uint8_t> > OpenixPacker::getFileDataByFilename(const std::string &filename) const {
    try {
        // Check if an image is loaded
        if (!imageLoaded_) {
            std::cerr << "Error: no image file loaded!" << std::endl;
            return std::nullopt;
        }

        // Search for the file by filename
        uint32_t numFiles = 0;

        if (imageHeader_.header_version == 0x0300) {
            numFiles = imageHeader_.v3.num_files;
        } else {
            numFiles = imageHeader_.v1.num_files;
        }

        for (uint32_t i = 0; i < numFiles; ++i) {
            auto *fileHeader = reinterpret_cast<const FileHeader *>(imageData_.data() + 1024 + i * 1024);
            const char *currentFilename = nullptr;
            uint32_t offset = 0;
            uint32_t originalLength = 0;

            if (imageHeader_.header_version == 0x0300) {
                currentFilename = fileHeader->v3.filename.data();
                offset = fileHeader->v3.offset;
                originalLength = fileHeader->v3.original_length;
            } else {
                currentFilename = fileHeader->v1.filename.data();
                offset = fileHeader->v1.offset;
                originalLength = fileHeader->v1.original_length;
            }

            if (currentFilename && std::strcmp(currentFilename, filename.c_str()) == 0) {
                if (verbose_) {
                    std::cout << "Extracting data for file: " << filename << " (" << originalLength << " bytes)" <<
                            std::endl;
                }

                // Extract file data (already decrypted during loadImage)
                std::vector<uint8_t> fileData;
                fileData.reserve(originalLength);
                fileData.assign(imageData_.data() + offset, imageData_.data() + offset + originalLength);

                return fileData;
            }
        }

        if (verbose_) {
            std::cout << "File data not found for: " << filename << std::endl;
        }

        // File not found
        return std::nullopt;
    } catch (const std::exception &e) {
        std::cerr << "Error retrieving file data: " << e.what() << std::endl;
        return std::nullopt;
    }
}

bool OpenixPacker::extractFileByFilename(const std::string &filename, const std::string &outputDir) const {
    try {
        // Check if an image is loaded
        if (!imageLoaded_) {
            std::cerr << "Error: no image file loaded!" << std::endl;
            return false;
        }

        // Get file data using getFileDataByFilename
        const auto fileDataOpt = getFileDataByFilename(filename);
        if (!fileDataOpt) {
            std::cerr << "Error: failed to get file data for " << filename << std::endl;
            return false;
        }

        const auto &fileData = *fileDataOpt;

        // Create directory structure if needed
        const std::string filePath = outputDir + "/" + filename;
        if (const size_t lastSlashPos = filePath.find_last_of("/\\"); lastSlashPos != std::string::npos) {
            if (const std::string dirPath = filePath.substr(0, lastSlashPos); !createDirectoryRecursive(dirPath)) {
                std::cerr << "Error: unable to create directory " << dirPath << "!" << std::endl;
                return false;
            }
        }

        // Write file data to output directory
        std::ofstream outFile(filePath, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Error: unable to create file " << filePath << "!" << std::endl;
            return false;
        }

        outFile.write(reinterpret_cast<const char *>(fileData.data()), fileData.size());
        outFile.close();

        if (verbose_) {
            std::cout << "Successfully extracted " << filename << " to " << outputDir << std::endl;
        }

        return true;
    } catch (const std::exception &e) {
        std::cerr << "Error extracting file: " << e.what() << std::endl;
        return false;
    }
}

bool OpenixPacker::createDirectoryRecursive(const std::string &dirPath) {
    try {
        return fs::create_directories(dirPath);
    } catch (const std::exception &e) {
        std::cerr << "Error creating directory " << dirPath << ": " << e.what() << std::endl;
        return false;
    }
}

std::FILE *OpenixPacker::openFileInDirectory(const std::string &dir, const std::string &path,
                                             const std::string &mode) {
    try {
        const std::string fullPath = dir + "/" + path;

        // Create directory structure if needed
        if (const size_t lastSlashPos = fullPath.find_last_of("/\\"); lastSlashPos != std::string::npos) {
            if (const std::string dirPath = fullPath.substr(0, lastSlashPos); !createDirectoryRecursive(dirPath)) {
                return nullptr;
            }
        }

        return std::fopen(fullPath.c_str(), mode.c_str());
    } catch (const std::exception &) {
        return nullptr;
    }
}

void *OpenixPacker::rc6EncryptInPlace(void *data, const size_t length, const RC6 &context) const {
    if (!encryptionEnabled_) {
        return static_cast<uint8_t *>(data) + length;
    }

    auto *current = static_cast<uint8_t *>(data);
    const auto numBlocks = length / 16;

    for (size_t i = 0; i < numBlocks; ++i) {
        context.encrypt(current);
        current += 16;
    }

    return current;
}

void *OpenixPacker::rc6DecryptInPlace(void *data, const size_t length, const RC6 &context) const {
    if (!encryptionEnabled_) {
        return static_cast<uint8_t *>(data) + length;
    }

    auto *current = static_cast<uint8_t *>(data);
    const auto numBlocks = length / 16;

    for (size_t i = 0; i < numBlocks; ++i) {
        context.decrypt(current);
        current += 16;
    }

    return current;
}

void *OpenixPacker::twofishDecryptInPlace(void *data, const size_t length, const Twofish &context) const {
    if (!encryptionEnabled_) {
        return static_cast<uint8_t *>(data) + length;
    }

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
