/**
 * @file imagewty.hpp
 * @brief Definition of Allwinner IMAGEWTY format structures for OpenixIMG
 * @author YuzukiTsuru <gloomyghost@gloomyghost.com>
 */

#ifndef IMAGEWTY_HPP
#define IMAGEWTY_HPP

#include <cstdint>
#include <array>
#include <string>

/**
 * @namespace OpenixIMG
 * @brief Main namespace for OpenixIMG library
 */
namespace OpenixIMG {
    /**
     * @brief Magic string identifier for ImageWTY format
     */
    constexpr auto IMAGEWTY_MAGIC = "IMAGEWTY";
    
    /**
     * @brief Length of the magic string
     */
    constexpr size_t IMAGEWTY_MAGIC_LEN = 8;
    
    /**
     * @brief Current version of the ImageWTY format
     */
    constexpr uint32_t IMAGEWTY_VERSION = 0x100234;
    
    /**
     * @brief Fixed length of the file header in bytes
     */
    constexpr size_t IMAGEWTY_FILEHDR_LEN = 1024;

    /**
     * @struct ImageHeader
     * @brief Main header structure for ImageWTY files
     * 
     * This structure contains metadata about the entire image file,
     * including version information, hardware IDs, and file count.
     */
    struct ImageHeader {
        /**
         * @brief Magic string identifier ("IMAGEWTY")
         */
        std::array<char, IMAGEWTY_MAGIC_LEN> magic{};
        
        /**
         * @brief Image header version (typically 0x0100 or 0x0300)
         */
        uint32_t header_version;
        
        /**
         * @brief Size of the header structure in bytes
         */
        uint32_t header_size;
        
        /**
         * @brief Base RAM address for the image
         */
        uint32_t ram_base;
        
        /**
         * @brief Format version (should be IMAGEWTY_VERSION)
         */
        uint32_t version;
        
        /**
         * @brief Total size of the image file (rounded up to 256 bytes)
         */
        uint32_t image_size;
        
        /**
         * @brief Size of the image header including padding
         */
        uint32_t image_header_size;

        /**
         * @brief Union containing version-specific header fields
         */
        union {
            /**
             * @brief Version 1 specific fields
             */
            struct {
                uint32_t pid;         //!< USB peripheral ID (from image.cfg)
                uint32_t vid;         //!< USB vendor ID (from image.cfg)
                uint32_t hardware_id; //!< Hardware ID (from image.cfg)
                uint32_t firmware_id; //!< Firmware ID (from image.cfg)
                uint32_t val1;        //!< Reserved value
                uint32_t val1024;     //!< Reserved value (typically 1024)
                uint32_t num_files;   //!< Total number of files embedded
                uint32_t val1024_2;   //!< Reserved value (typically 1024)
                uint32_t val0;        //!< Reserved value (typically 0)
                uint32_t val0_2;      //!< Reserved value (typically 0)
                uint32_t val0_3;      //!< Reserved value (typically 0)
                uint32_t val0_4;      //!< Reserved value (typically 0)
            } v1;

            /**
             * @brief Version 3 specific fields
             */
            struct {
                uint32_t unknown;     //!< Unknown value
                uint32_t pid;         //!< USB peripheral ID (from image.cfg)
                uint32_t vid;         //!< USB vendor ID (from image.cfg)
                uint32_t hardware_id; //!< Hardware ID (from image.cfg)
                uint32_t firmware_id; //!< Firmware ID (from image.cfg)
                uint32_t val1;        //!< Reserved value
                uint32_t val1024;     //!< Reserved value (typically 1024)
                uint32_t num_files;   //!< Total number of files embedded
                uint32_t val1024_2;   //!< Reserved value (typically 1024)
                uint32_t val0;        //!< Reserved value (typically 0)
                uint32_t val0_2;      //!< Reserved value (typically 0)
                uint32_t val0_3;      //!< Reserved value (typically 0)
                uint32_t val0_4;      //!< Reserved value (typically 0)
            } v3;
        };

        /**
         * @brief Default constructor
         * 
         * Initializes all fields to zero and sets the magic string.
         */
        ImageHeader();

        /**
         * @brief Initializes the image header with specified parameters
         * 
         * @param _version Format version
         * @param pid USB peripheral ID
         * @param vid USB vendor ID
         * @param hardware_id Hardware ID
         * @param firmware_id Firmware ID
         * @param num_files Number of embedded files
         */
        void initialize(uint32_t _version, uint32_t pid, uint32_t vid, uint32_t hardware_id, uint32_t firmware_id,
                        uint32_t num_files);
    };

    /**
     * @brief Length of the main type field in the file header
     */
    constexpr size_t IMAGEWTY_FHDR_MAINTYPE_LEN = 8;
    
    /**
     * @brief Length of the subtype field in the file header
     */
    constexpr size_t IMAGEWTY_FHDR_SUBTYPE_LEN = 16;
    
    /**
     * @brief Length of the filename field in the file header
     */
    constexpr size_t IMAGEWTY_FHDR_FILENAME_LEN = 256;

    /**
     * @struct FileHeader
     * @brief Header structure for individual files within an ImageWTY file
     * 
     * This structure contains metadata about each embedded file,
     * including type information, size, and location.
     */
    struct FileHeader {
        /**
         * @brief Length of the filename
         */
        uint32_t filename_len;
        
        /**
         * @brief Total size of the file header
         */
        uint32_t total_header_size;
        
        /**
         * @brief Main type identifier for the file
         */
        std::array<char, IMAGEWTY_FHDR_MAINTYPE_LEN> maintype;
        
        /**
         * @brief Subtype identifier for the file
         */
        std::array<char, IMAGEWTY_FHDR_SUBTYPE_LEN> subtype;

        /**
         * @brief Union containing version-specific file header fields
         */
        union {
            /**
             * @brief Version 1 specific file fields
             */
            struct {
                uint32_t unknown_3;     //!< Unknown value
                uint32_t stored_length; //!< Length of the stored (possibly compressed) file
                uint32_t original_length; //!< Original length of the file
                uint32_t offset;        //!< Offset to the file data from the start of the image
                uint32_t unknown;       //!< Unknown value
                std::array<char, IMAGEWTY_FHDR_FILENAME_LEN> filename; //!< Filename
            } v1;

            /**
             * @brief Version 3 specific file fields
             */
            struct {
                uint32_t unknown_0;     //!< Unknown value
                std::array<char, IMAGEWTY_FHDR_FILENAME_LEN> filename; //!< Filename
                uint32_t stored_length; //!< Length of the stored (possibly compressed) file
                uint32_t pad1;          //!< Padding
                uint32_t original_length; //!< Original length of the file
                uint32_t pad2;          //!< Padding
                uint32_t offset;        //!< Offset to the file data from the start of the image
            } v3;
        };

        /**
         * @brief Default constructor
         * 
         * Initializes fields to default values.
         */
        FileHeader();

        /**
         * @brief Initializes the file header with specified parameters
         * 
         * @param filename Name of the file
         * @param maintype_ Main type identifier
         * @param subtype_ Subtype identifier
         * @param size Size of the file
         * @param offset Offset to the file data
         */
        void initialize(const std::string &filename, const std::string &maintype_, const std::string &subtype_,
                        uint32_t size, uint32_t offset);
    };
} // namespace OpenixIMG

#endif // IMAGEWTY_HPP
