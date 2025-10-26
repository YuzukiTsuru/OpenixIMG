/**
 * @file OpenixPartition.hpp
 * @brief Header file for the OpenixPartition class, responsible for parsing partition table files.
 * @author YuzukiTsuru
 * @date 2025/10/8
 */

#ifndef OPENIXIMG_OPENIXPARTITION_HPP
#define OPENIXIMG_OPENIXPARTITION_HPP

#include <string>
#include <vector>
#include <memory>
#include <filesystem>

namespace fs = std::filesystem;

namespace OpenixIMG {
    /**
     * @struct Partition
     * @brief Structure to store partition information.
     */
    struct Partition {
        std::string name; ///< Partition name
        uint64_t size; ///< Partition size (unit: sectors)
        std::string downloadfile; ///< Download file path
        uint32_t user_type; ///< User type
        bool keydata; ///< Whether it's a private data partition
        bool encrypt; ///< Whether it's encrypted
        bool verify; ///< Whether verification is required
        bool ro; ///< Whether it's read-only

        /**
         * @brief Default constructor for Partition.
         * Initializes all member variables to default values.
         */
        Partition() : size(0), user_type(0), keydata(false), encrypt(false), verify(false), ro(false) {
        }
    };

    /**
     * @class OpenixPartition
     * @brief Class to parse and manage partition table information from sys_partition.fex files.
     */
    class OpenixPartition {
    public:
        /**
         * @brief Default constructor for OpenixPartition.
         * Initializes member variables to default values.
         */
        OpenixPartition();

        /**
         * @brief Default destructor for OpenixPartition.
         */
        ~OpenixPartition();

        /**
         * @brief Parse partition table from a file.
         * @param filePath Path to the partition table file (usually sys_partition.fex).
         * @return True if parsing was successful, false otherwise.
         */
        bool parseFromFile(const std::string &filePath);

        /**
         * @brief Parse partition table from memory data.
         * @param data Pointer to the memory data containing partition table information.
         * @param size Size of the data in bytes.
         * @return True if parsing was successful, false otherwise.
         */
        bool parseFromData(const uint8_t *data, size_t size);

        /**
         * @brief Get the MBR size.
         * @return MBR size in KB.
         */
        [[nodiscard]] uint32_t getMbrSize() const;

        /**
         * @brief Get all partitions.
         * @return Constant reference to the vector of partitions.
         */
        [[nodiscard]] const std::vector<Partition> &getPartitions() const;

        /**
         * @brief Get a partition by name.
         * @param name Name of the partition to find.
         * @return Shared pointer to the found partition, or nullptr if not found.
         */
        [[nodiscard]] std::shared_ptr<Partition> getPartitionByName(const std::string &name) const;

        /**
         * @brief Check if a partition name exists.
         * @param name Name of the partition to check.
         * @return True if the partition name exists, false otherwise.
         */
        [[nodiscard]] bool isPartitionNameExists(const std::string &name) const;

        /**
         * @brief Dump the partition table information to a string.
         * @return A string containing the formatted partition table information.
         */
        [[nodiscard]] std::string dumpToString() const;

        /**
         * @brief Dump the partition table information to standard output.
         */
        void dump() const;

        /**
         * @brief Dump the partition table information to a JSON string.
         * @return A JSON string containing the partition table information.
         */
        [[nodiscard]] std::string dumpToJson() const;

    private:
        uint32_t mbrSize; ///< MBR size in KB
        std::vector<Partition> partitions; ///< List of partitions

        /**
         * @brief Helper method to parse partition table from an input stream.
         * @param stream Input stream containing partition table data.
         * @return True if parsing was successful, false otherwise.
         */
        bool parseFromStream(std::istream &stream);

        /**
         * @brief Parse a single configuration line.
         * @param line The line to parse.
         * @param partition Reference to the partition to store the parsed data.
         * @return True if parsing was successful, false otherwise.
         */
        static bool parseLine(const std::string &line, Partition &partition);

        /**
         * @brief Skip whitespace characters in a line.
         * @param line The line to process.
         * @param pos Current position in the line, will be updated.
         */
        static void skipWhitespace(const std::string &line, size_t &pos);

        /**
         * @brief Parse an identifier from a line.
         * @param line The line to parse.
         * @param pos Current position in the line, will be updated.
         * @return The parsed identifier.
         */
        static std::string parseIdentifier(const std::string &line, size_t &pos);

        /**
         * @brief Parse a string from a line.
         * @param line The line to parse.
         * @param pos Current position in the line, will be updated.
         * @return The parsed string.
         */
        static std::string parseString(const std::string &line, size_t &pos);

        /**
         * @brief Parse a number from a line.
         * @param line The line to parse.
         * @param pos Current position in the line, will be updated.
         * @return The parsed number as uint64_t.
         */
        static uint64_t parseNumber(const std::string &line, size_t &pos);
    };
}


#endif //OPENIXIMG_OPENIXPARTITION_HPP
