/**
 * @file OpenixIMGFile.hpp
 * @brief Header file for OpenixIMGFile class responsible for managing image file data and structure
 * @author YuzukiTsuru <gloomyghost@gloomyghost.com>
 */

#ifndef OPENIXIMG_OPENIXIMG_HPP
#define OPENIXIMG_OPENIXIMG_HPP

// Provide ssize_t definition for Windows platform
#ifdef _WIN32
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include <string>
#include <vector>
#include <optional>

#include "rc6.hpp"
#include "twofish.hpp"

#include "OpenixIMGWTY.hpp"

/**
 * @namespace OpenixIMG
 * @brief Main namespace for OpenixIMG library
 */
namespace OpenixIMG {
    /**
     * @class OpenixIMGFile
     * @brief Class responsible for loading, parsing, and managing image file data and structure
     * 
     * This class provides functionality to load image files, parse their structure,
     * and provide access to the contained files and metadata.
     */
    class OpenixIMGFile {
    public:
        // File information structure for file list
        struct FileInfo {
            std::string filename;
            std::string maintype;
            std::string subtype;
            uint32_t storedLength;
            uint32_t originalLength;
            uint32_t offset;
        };

        /**
         * @brief Default constructor
         * 
         * Initializes the image file handler with default settings without loading an image file.
         */
        OpenixIMGFile();

        /**
         * @brief Constructor with image file path
         * 
         * Initializes the image file handler and loads the image file, parsing header information.
         * 
         * @param imageFilePath Path to the image file to load and parse
         */
        explicit OpenixIMGFile(const std::string &imageFilePath);

        /**
         * @brief Default destructor
         */
        ~OpenixIMGFile();

        /**
         * @brief Load and parse an image file
         * 
         * @param imageFilePath Path to the image file to load and parse
         * @return True if loading and parsing was successful, false otherwise
         */
        bool loadImage(const std::string &imageFilePath);

        /**
         * @brief Enable or disable encryption
         * 
         * @param enabled True to enable encryption, false to disable
         */
        void setEncryptionEnabled(bool enabled);


        /**
         * @brief Initialize cryptographic contexts
         * 
         * Sets up the encryption contexts for header, file headers, and file content.
         */
        void initializeCrypto();

        /**
         * @brief Get the loaded image file path
         * 
         * @return The path to the loaded image file
         */
        [[nodiscard]] std::string getImageFilePath() const;

        /**
         * @brief Check if an image file is currently loaded
         * 
         * @return True if an image file is loaded, false otherwise
         */
        [[nodiscard]] bool isImageLoaded() const;

        /**
         * @brief Free the loaded image data and reset state
         * 
         * Releases memory occupied by the loaded image data and resets
         * the state to indicate no image is loaded.
         */
        void freeImage();

        /**
         * @brief Reload the currently loaded image file with a new path
         * 
         * Loads a new image file, replacing the currently loaded image if any.
         *
         */
        void reloadImage();

        /**
         * @brief Reload the currently loaded image file with a new path
         * 
         * Loads a new image file, replacing the currently loaded image if any.
         * 
         * @param newImageFilePath Path to the new image file to load
         * @return True if reloading was successful, false otherwise
         */
        void reloadImage(const std::string &newImageFilePath);

        /**
         * @brief Get the Product ID (PID)
         * 
         * @return PID of the loaded image
         */
        [[nodiscard]] uint32_t getPID() const;

        /**
         * @brief Get the Vendor ID (VID)
         * 
         * @return VID of the loaded image
         */
        [[nodiscard]] uint32_t getVID() const;

        /**
         * @brief Get the Hardware ID
         * 
         * @return Hardware ID of the loaded image
         */
        [[nodiscard]] uint32_t getHardwareId() const;

        /**
         * @brief Get the Firmware ID
         * 
         * @return Firmware ID of the loaded image
         */
        [[nodiscard]] uint32_t getFirmwareId() const;

        /**
         * @brief Check if a file exists in the loaded image by filename
         * 
         * @param filename Name of the file to check
         * @return True if the file exists, false otherwise
         */
        [[nodiscard]] bool checkFileByFilename(const std::string &filename) const;

        /**
         * @brief Check if a file exists in the loaded image by subtype
         * 
         * @param subtype Subtype of the file to check
         * @return True if a file with the specified subtype exists, false otherwise
         */
        [[nodiscard]] bool checkFileBySubtype(const std::string &subtype) const;

        /**
         * @brief Get file header information from the loaded image by filename
         * 
         * @param filename Name of the file to check
         * @return File header information if found, std::nullopt otherwise
         */
        [[nodiscard]] std::optional<FileHeader> getFileHeaderByFilename(const std::string &filename) const;

        /**
         * @brief Get file header information from the loaded image by subtype
         * 
         * @param subtype Subtype of the file to check
         * @return Vector of file header information for all files with the specified subtype
         */
        [[nodiscard]] std::vector<FileHeader> getFileHeaderBySubtype(const std::string &subtype) const;

        // Extract methods removed, use getFileDataByFilename and getFileDataBySubtype instead

        /**
         * @brief Get file data from the loaded image by filename
         * 
         * @param filename Name of the file to extract
         * @return Vector containing the file data if found, std::nullopt otherwise
         */
        [[nodiscard]] std::optional<std::vector<uint8_t> > getFileDataByFilename(const std::string &filename) const;

        /**
         * @brief Get file data from the loaded image by subtype
         * 
         * @param subtype Subtype of the files to extract
         * @return Vector of pairs containing filename and file data for all files with the specified subtype
         */
        [[nodiscard]] std::vector<std::pair<std::string, std::vector<uint8_t> > > getFileDataBySubtype(
            const std::string &subtype) const;

        /**
         * @brief Get the loaded image data
         * 
         * @return Reference to the image data vector
         */
        [[nodiscard]] const std::vector<uint8_t> &getImageData() const;

        /**
         * @brief Get the image header
         * 
         * @return Reference to the image header structure
         */
        [[nodiscard]] const ImageHeader &getImageHeader() const;

        /**
         * @brief Get the list of files in the image
         * 
         * @return Reference to the file list vector
         */
        [[nodiscard]] const std::vector<FileInfo> &getFileList() const;

        /**
         * @brief Check if the image is encrypted
         * 
         * @return True if the image is encrypted, false otherwise
         */
        [[nodiscard]] bool isEncrypted() const;

    private:
        /**
         * @brief RC6 encrypt data in place
         * 
         * @param data Pointer to data to encrypt
         * @param length Length of data
         * @param context RC6 context to use
         * @return Pointer to encrypted data (same as input pointer)
         */
        static void *rc6EncryptInPlace(void *data, size_t length, const RC6 &context);

        /**
         * @brief RC6 decrypt data in place
         * 
         * @param data Pointer to data to decrypt
         * @param length Length of data
         * @param context RC6 context to use
         * @return Pointer to decrypted data (same as input pointer)
         */
        static void *rc6DecryptInPlace(void *data, size_t length, const RC6 &context);

        /**
         * @brief Twofish decrypt data in place
         * 
         * @param data Pointer to data to decrypt
         * @param length Length of data
         * @param context Twofish context to use
         * @return Pointer to decrypted data (same as input pointer)
         */
        static void *twofishDecryptInPlace(void *data, size_t length, const Twofish &context);


        // Private helper function to load file list
        void loadFileList();

        // Member variables
        bool encryptionEnabled_; //!< Flag indicating if encryption is enabled
        bool imageLoaded_; //!< Flag indicating if an image file is loaded
        std::string imageFilePath_; //!< Path to the loaded image file
        ssize_t imageSize_; //!< Size of the loaded image file
        std::vector<uint8_t> imageData_; //!< Content of the loaded image file
        ImageHeader imageHeader_; //!< Parsed image header
        bool isEncrypted_{}; //!< Flag indicating if the image is encrypted
        std::vector<FileInfo> fileList_; //!< List of files in the image

        // Image metadata
        uint32_t pid_{}; //!< Product ID
        uint32_t vid_{}; //!< Vendor ID
        uint32_t hardwareId_{}; //!< Hardware ID
        uint32_t firmwareId_{}; //!< Firmware ID

        // Crypto contexts
        RC6 headerContext_; //!< RC6 context for header encryption
        RC6 fileHeadersContext_; //!< RC6 context for file headers encryption
        RC6 fileContentContext_; //!< RC6 context for file content encryption
        Twofish twofishContext_; //!< Twofish context for decryption
        std::vector<uint8_t> twofishKey_; //!< Twofish encryption key
    };
}

#endif //OPENIXIMG_OPENIXIMG_HPP
