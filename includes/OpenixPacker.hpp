/**
 * @file OpenixPacker.hpp
 * @brief Header file for OpenixPacker class responsible for packing and unpacking image files
 * @author YuzukiTsuru <gloomyghost@gloomyghost.com>
 */

#ifndef OPENIXPACKER_HPP
#define OPENIXPACKER_HPP

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
     * @enum OutputFormat
     * @brief Enumeration of supported output formats for image packing
     */
    enum class OutputFormat {
        IMGREPACKER, //!< IMGREPACKER format
        UNIMG //!< UNIMG format
    };

    /**
     * @class OpenixPacker
     * @brief Class responsible for packing, unpacking, and decrypting image files
     * 
     * This class provides functionality to create image files from directories,
     * unpack image files to directories, and decrypt encrypted image files.
     */
    class OpenixPacker {
    public:
        /**
         * @brief Default constructor (deprecated)
         * 
         * Initializes the image packer with default settings without loading an image file.
         */
        OpenixPacker();

        /**
         * @brief Constructor with image file path
         * 
         * Initializes the image packer and loads the image file, parsing header information.
         * 
         * @param imageFilePath Path to the image file to load and parse
         */
        explicit OpenixPacker(const std::string &imageFilePath);

        /**
         * @brief Default destructor
         */
        ~OpenixPacker();

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
         * @brief Set the output format for packing
         * 
         * @param format Output format to use
         */
        void setOutputFormat(OutputFormat format);

        /**
         * @brief Set verbose mode
         * 
         * @param verbose True to enable verbose output, false to disable
         */
        void setVerbose(bool verbose);

        /**
         * @brief Initialize cryptographic contexts
         * 
         * Sets up the encryption contexts for header, file headers, and file content.
         */
        void initializeCrypto();

        /**
         * @brief Pack a directory into an image file
         * 
         * @param inputDir Path to the input directory
         * @param outputFile Path to the output image file
         * @return True if packing was successful, false otherwise
         */
        [[nodiscard]] static bool packImage(const std::string &inputDir, const std::string &outputFile);

        /**
         * @brief Decrypt an encrypted image file
         * 
         * @param outputFile Path to the output decrypted image file
         * @return True if decryption was successful, false otherwise
         */
        [[nodiscard]] bool decryptImage(const std::string &outputFile) const;

        /**
         * @brief Unpack the loaded image file to a directory
         * 
         * @param outputDir Path to the output directory
         * @return True if unpacking was successful, false otherwise
         */
        [[nodiscard]] bool unpackImage(const std::string &outputDir) const;

        /**
         * @brief Check if a file exists in the loaded image by filename
         * 
         * @param filename Name of the file to check
         * @return True if the file exists, false otherwise
         */
        [[nodiscard]] bool checkFileByFilename(const std::string &filename) const;

        /**
         * @brief Get file header information from the loaded image by filename
         * 
         * @param filename Name of the file to check
         * @return File header information if found, std::nullopt otherwise
         */
        [[nodiscard]] std::optional<FileHeader> getFileHeaderByFilename(const std::string &filename) const;

        /**
         * @brief Extract a file from the loaded image by filename
         * 
         * @param filename Name of the file to extract
         * @param outputDir Path to save the extracted file
         * @return True if extraction was successful, false otherwise
         */
        bool extractFileByFilename(const std::string &filename,
                                   const std::string &outputDir) const;

        /**
         * @brief Get file data from the loaded image by filename
         * 
         * @param filename Name of the file to extract
         * @return Vector containing the file data if found, std::nullopt otherwise
         */
        [[nodiscard]] std::optional<std::vector<uint8_t>> getFileDataByFilename(const std::string &filename) const;

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
          * @return True if reloading was successful, false otherwise
          */
         bool reloadImage();

        /**
          * @brief Reload the currently loaded image file with a new path
          * 
          * Loads a new image file, replacing the currently loaded image if any.
          * 
          * @param newImageFilePath Path to the new image file to load
          * @return True if reloading was successful, false otherwise
          */
         bool reloadImage(const std::string &newImageFilePath);

    private:
        /**
         * @brief Create directories recursively
         * 
         * @param dirPath Path to the directory to create
         * @return True if directory creation was successful, false otherwise
         */
        static bool createDirectoryRecursive(const std::string &dirPath);

        /**
         * @brief Open a file in a directory, creating directories as needed
         * 
         * @param dir Base directory path
         * @param path Relative file path
         * @param mode File open mode
         * @return File pointer if successful, nullptr otherwise
         */
        static std::FILE *openFileInDirectory(const std::string &dir, const std::string &path, const std::string &mode);

        /**
         * @brief RC6 encrypt data in place
         * 
         * @param data Pointer to data to encrypt
         * @param length Length of data
         * @param context RC6 context to use
         * @return Pointer to encrypted data (same as input pointer)
         */
        void *rc6EncryptInPlace(void *data, size_t length, const RC6 &context) const;

        /**
         * @brief RC6 decrypt data in place
         * 
         * @param data Pointer to data to decrypt
         * @param length Length of data
         * @param context RC6 context to use
         * @return Pointer to decrypted data (same as input pointer)
         */
        void *rc6DecryptInPlace(void *data, size_t length, const RC6 &context) const;

        /**
         * @brief Twofish decrypt data in place
         * 
         * @param data Pointer to data to decrypt
         * @param length Length of data
         * @param context Twofish context to use
         * @return Pointer to decrypted data (same as input pointer)
         */
        void *twofishDecryptInPlace(void *data, size_t length, const Twofish &context) const;

        // Member variables
        bool encryptionEnabled_; //!< Flag indicating if encryption is enabled
        OutputFormat outputFormat_; //!< Current output format
        bool verbose_; //!< Flag indicating if verbose output is enabled
        bool imageLoaded_; //!< Flag indicating if an image file is loaded
        std::string imageFilePath_; //!< Path to the loaded image file
        ssize_t imageSize_; //!< Size of the loaded image file
        std::vector<uint8_t> imageData_; //!< Content of the loaded image file
        ImageHeader imageHeader_; //!< Parsed image header
        bool isEncrypted_; //!< Flag indicating if the image is encrypted

        // Crypto contexts
        RC6 headerContext_; //!< RC6 context for header encryption
        RC6 fileHeadersContext_; //!< RC6 context for file headers encryption
        RC6 fileContentContext_; //!< RC6 context for file content encryption
        Twofish twofishContext_; //!< Twofish context for decryption
        std::vector<uint8_t> twofishKey_; //!< Twofish encryption key
    };
} // namespace OpenixIMG

#endif /* IMAGEPACKER_HPP */
