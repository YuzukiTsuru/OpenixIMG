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
#include <algorithm>
#include <cstdint>

#include "OpenixIMGWTY.hpp"
#include "OpenixPacker.hpp"

#include "OpenixCFG.hpp"
#include "OpenixUtils.hpp"

using namespace OpenixIMG;
namespace fs = std::filesystem;

OpenixPacker::OpenixPacker(OpenixIMGFile &imgFile) : imgFile_(imgFile) {
}

OpenixPacker::~OpenixPacker() = default;

bool OpenixPacker::genImageCfgFromFileList(const std::vector<OpenixIMGFile::FileInfo> &fileList,
                                           const std::string &outputDir, const OutputFormat &outputFormat) const {
    try {
        // Create OpenixCFG instance to build the configuration
        OpenixCFG cfg;

        // Create DIR_DEF group
        const auto dirDefGroup = std::make_shared<Group>("DIR_DEF");
        const auto inputDirVar = std::make_shared<Variable>("INPUT_DIR", ValueType::STRING);
        inputDirVar->setString("../");
        dirDefGroup->addVariable(inputDirVar);

        auto fileListGroup = std::make_shared<Group>("FILELIST");
        for (const auto &fileInfo: fileList) {
            auto listItem = std::make_shared<Variable>("", ValueType::LIST_ITEM);

            // Determine filename based on output format
            std::string filename;
            if (outputFormat == OutputFormat::UNIMG) {
                filename = fileInfo.maintype + "_" + fileInfo.subtype;
            } else {
                filename = fileInfo.filename;
                // Remove leading slash if present
                if (!filename.empty() && filename[0] == '/') {
                    filename = filename.substr(1);
                }
            }

            auto filenameVar = std::make_shared<Variable>("filename", ValueType::STRING);
            filenameVar->setString(filename);
            listItem->addItem(filenameVar);

            // Add maintype to list item
            auto maintypeVar = std::make_shared<Variable>("maintype", ValueType::STRING);
            maintypeVar->setString(fileInfo.maintype);
            listItem->addItem(maintypeVar);

            // Add subtype to list item
            auto subtypeVar = std::make_shared<Variable>("subtype", ValueType::STRING);
            subtypeVar->setString(fileInfo.subtype);
            listItem->addItem(subtypeVar);

            // Add list item to FILELIST group
            fileListGroup->addVariable(listItem);
        }

        // Create IMAGE_CFG group
        auto imageCfgGroup = std::make_shared<Group>("IMAGE_CFG");

        // Add basic image configuration
        auto versionVar = std::make_shared<Variable>("version", ValueType::NUMBER);
        versionVar->setNumber(1);
        imageCfgGroup->addVariable(versionVar);

        // Add pid to IMAGE_CFG
        auto pidVar = std::make_shared<Variable>("pid", ValueType::NUMBER);
        pidVar->setNumber(imgFile_.getPID());
        imageCfgGroup->addVariable(pidVar);

        // Add vid to IMAGE_CFG
        auto vidVar = std::make_shared<Variable>("vid", ValueType::NUMBER);
        vidVar->setNumber(imgFile_.getVID());
        imageCfgGroup->addVariable(vidVar);

        // Add hardwareid to IMAGE_CFG
        auto hardwareidVar = std::make_shared<Variable>("hardwareid", ValueType::NUMBER);
        hardwareidVar->setNumber(imgFile_.getHardwareId());
        imageCfgGroup->addVariable(hardwareidVar);

        // Add firmwareid to IMAGE_CFG
        auto firmwareidVar = std::make_shared<Variable>("firmwareid", ValueType::NUMBER);
        firmwareidVar->setNumber(imgFile_.getFirmwareId());
        imageCfgGroup->addVariable(firmwareidVar);

        // Add imagename to IMAGE_CFG
        auto imagenameVar = std::make_shared<Variable>("imagename", ValueType::REFERENCE);
        imagenameVar->setReference(imgFile_.getImageFilePath());
        imageCfgGroup->addVariable(imagenameVar);

        // Add filelist to IMAGE_CFG
        auto filelistVar = std::make_shared<Variable>("filelist", ValueType::REFERENCE);
        filelistVar->setReference("FILELIST");
        imageCfgGroup->addVariable(filelistVar);

        // Add groups to OpenixCFG instance
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
        configFile << "; " << imgFile_.getImageFilePath() << "\n";
        configFile << ";/**************************************************************************/\n";

        // Use dumpToString() to get configuration content and write to file
        std::string configContent = cfg.dumpToString();
        configFile << configContent;

        configFile.close();

        return true;
    } catch (const std::exception &e) {
        std::cerr << "Error generating image configuration: " << e.what() << std::endl;
        return false;
    }
}

bool OpenixPacker::unpackImage(const std::string &outputDir, const OutputFormat &outputFormat) const {
    try {
        // Check if image is loaded
        if (!imgFile_.isImageLoaded()) {
            throw std::runtime_error("No image file loaded!");
        }

        OpenixUtils::log("Unpacking image to " + outputDir);
        OpenixUtils::log(
            "Output format: " + std::string(outputFormat == OutputFormat::UNIMG ? "UNIMG" : "IMGREPACKER"));

        // Recreate output directory if it exists
        if (fs::exists(outputDir)) {
            if (!fs::remove_all(outputDir)) {
                std::cerr << "Error: unable to remove existing output directory " << outputDir << "!" << std::endl;
                return false;
            }
        }

        // Create output directory
        if (!fs::create_directories(outputDir)) {
            throw std::runtime_error("Cannot create output directory: " + outputDir + "!");
        }

        // Extract all files from the image
        const auto &fileList = imgFile_.getFileList();
        bool allSuccess = true;

        for (const auto &fileInfo: fileList) {
            if (OutputFormat::UNIMG == outputFormat) {
                // UNIMG format output
                OpenixUtils::log("Extracting: " + fileInfo.maintype + " " + fileInfo.subtype);

                // Get file data from memory
                auto fileDataOpt = imgFile_.getFileDataByFilename(fileInfo.filename);
                if (!fileDataOpt) {
                    throw std::runtime_error("Error extracting file data: " + fileInfo.filename);
                }

                const auto &fileData = *fileDataOpt;

                // Create header and content filenames
                std::string hdrFilename = fileInfo.maintype + "_" + fileInfo.subtype + ".hdr";
                std::string contFilename = fileInfo.maintype + "_" + fileInfo.subtype;

                // Write content file
                std::string contPath = outputDir + std::string("/").append(contFilename);
                std::ofstream contFile(contPath, std::ios::binary);
                if (!contFile.is_open()) {
                    throw std::runtime_error("Unable to create content file: " + contPath);
                }
                contFile.write(reinterpret_cast<const char *>(fileData.data()),
                               static_cast<std::streamsize>(fileData.size()));
                contFile.close();
            } else if (OutputFormat::IMGREPACKER == outputFormat) {
                // IMGREPACKER format output
                OpenixUtils::log("Extracting " + fileInfo.filename);

                // Get file data from memory
                auto fileDataOpt = imgFile_.getFileDataByFilename(fileInfo.filename);
                if (!fileDataOpt) {
                    std::cerr << "Error extracting file data: " << fileInfo.filename << std::endl;
                    allSuccess = false;
                    continue;
                }

                const auto &fileData = *fileDataOpt;

                // Write file data to output directory
                std::string outFilePath = outputDir + std::string("/").append(fileInfo.filename);
                std::ofstream outFile(outFilePath, std::ios::binary);
                if (!outFile.is_open()) {
                    throw std::runtime_error("Unable to create file: " + outFilePath);
                }

                outFile.write(reinterpret_cast<const char *>(fileData.data()),
                              static_cast<std::streamsize>(fileData.size()));
                outFile.close();
            }
        }

        if (!genImageCfgFromFileList(fileList, outputDir, outputFormat)) {
            throw std::runtime_error("Failed to generate image configuration files!");
        }
        OpenixUtils::log("Successfully unpacked " + std::to_string(fileList.size()) + " files to " + outputDir);

        return allSuccess;
    } catch (const std::exception &) {
        throw;
    }
}
