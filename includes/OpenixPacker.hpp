/**
 * @file OpenixPacker.hpp
 * @brief Openix IMG packer interface
 * @author YuzukiTsuru <gloomyghost@gloomyghost.com>
 */

#pragma once

#include <string>

#include "OpenixIMGWTY.hpp"
#include "OpenixIMGFile.hpp"

namespace OpenixIMG {
    enum class OutputFormat {
        UNIMG,
        IMGREPACKER
    };

    /**
     * @brief The OpenixPacker class provides high-level image packing, unpacking and decryption operations.
     * It uses OpenixIMGFile for low-level image operations and structure management.
     */
    class OpenixPacker {
    public:
        explicit OpenixPacker(OpenixIMGFile &imgFile);

        ~OpenixPacker();

        [[nodiscard]] bool unpackImage(const std::string &outputDir, const OutputFormat &outputFormat) const;

    private:
        [[nodiscard]] bool genImageCfgFromFileList(const std::vector<OpenixIMGFile::FileInfo> &fileList,
                                                   const std::string &outputDir,
                                                   const OutputFormat &outputFormat) const;
private:
        OpenixIMGFile &imgFile_; //!< Reference to IMG file handler
    };
} // namespace OpenixIMG
