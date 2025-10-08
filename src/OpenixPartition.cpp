/**
* @file OpenixPartition.cpp
 * @brief Implementation of OpenixPartition class methods
 * @author YuzukiTsuru <gloomyghost@gloomyghost.com>
 */

#include <fstream>
#include <cctype>
#include <sstream>
#include <algorithm>
#include <iostream>

#include "OpenixPartition.hpp"

using namespace OpenixIMG;

OpenixPartition::OpenixPartition() : mbrSize(0) {
}

OpenixPartition::~OpenixPartition() = default;

bool OpenixPartition::parseFromFile(const std::string &filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    bool result = parseFromStream(file);
    file.close();
    return result;
}

bool OpenixPartition::parseFromData(const uint8_t *data, const size_t size) {
    if (!data || size == 0) {
        return false;
    }

    // Convert memory data to a string
    const std::string content(reinterpret_cast<const char *>(data), size);
    std::istringstream fileStream(content);
    
    return parseFromStream(fileStream);
}

bool OpenixPartition::parseFromStream(std::istream &stream) {
    std::string line;
    bool inMbrSection = false;
    bool inPartitionSection = false;
    Partition currentPartition;

    while (std::getline(stream, line)) {
        // Remove leading and trailing whitespace and \r characters
        line.erase(0, line.find_first_not_of(" \t\r"));
        line.erase(line.find_last_not_of(" \t\r") + 1);

        // Skip empty lines and comments
        if (line.empty() || line[0] == ';' || line.substr(0, 2) == "//") {
            continue;
        }

        // Check if it's the start of partitions section
        if (line == "[partition_start]") {
            inPartitionSection = true;
            inMbrSection = false;
            continue;
        }

        // Check if it's the MBR section
        if (line == "[mbr]") {
            inMbrSection = true;
            inPartitionSection = false;
            continue;
        }

        // Check if it's a new partition
        if (line == "[partition]") {
            inMbrSection = false;

            // Save the current partition if it's not empty
            if (!currentPartition.name.empty()) {
                partitions.push_back(currentPartition);
            }

            // Reset current partition
            currentPartition = Partition();
            inPartitionSection = true;
            continue;
        }

        // Parse MBR size
        if (inMbrSection) {
            size_t pos = 0;
            skipWhitespace(line, pos);

            if (pos < line.size() && line.compare(pos, 4, "size") == 0) {
                pos += 4;
                skipWhitespace(line, pos);

                if (pos < line.size() && line[pos] == '=') {
                    pos++;
                    skipWhitespace(line, pos);

                    if (pos < line.size()) {
                        mbrSize = static_cast<uint32_t>(parseNumber(line, pos));
                    }
                }
            }
        }

        // Parse partition information
        if (inPartitionSection && !currentPartition.name.empty()) {
            parseLine(line, currentPartition);
        }

        // Parse the name of the first partition
        if (inPartitionSection && currentPartition.name.empty() && line.find("name") != std::string::npos) {
            parseLine(line, currentPartition);
        }
    }

    // Save the last partition
    if (inPartitionSection && !currentPartition.name.empty()) {
        partitions.push_back(currentPartition);
    }

    return true;
}

uint32_t OpenixPartition::getMbrSize() const {
    return mbrSize;
}

const std::vector<Partition> &OpenixPartition::getPartitions() const {
    return partitions;
}

std::shared_ptr<Partition> OpenixPartition::getPartitionByName(const std::string &name) const {
    const auto it = std::find_if(partitions.begin(), partitions.end(),
                                 [&name](const Partition &p) { return p.name == name; });

    if (it != partitions.end()) {
        return std::make_shared<Partition>(*it);
    }

    return nullptr;
}

bool OpenixPartition::isPartitionNameExists(const std::string &name) const {
    return std::any_of(partitions.begin(), partitions.end(),
                       [&name](const Partition &p) { return p.name == name; });
}

std::string OpenixPartition::dumpToString() const {
    std::stringstream ss;
    // Print all partitions information
    ss << "\nPartition details:" << std::endl;
    ss << "--------------------------------------------------------------------------------------------------------"
            << std::endl;
    ss << std::left << std::setw(20) << "Name" << std::setw(20) << "Size" << std::setw(35) << "Download File" <<
            std::setw(10) << "User Type" << "Flags" << std::endl;
    ss << "--------------------------------------------------------------------------------------------------------"
            << std::endl;

    for (const auto &partition: partitions) {
        ss << std::left << std::setw(20) << partition.name
                << std::setw(20) << partition.size;

        if (!partition.downloadfile.empty()) {
            ss << std::setw(35) << partition.downloadfile;
        } else {
            ss << std::setw(35) << "-";
        }

        std::stringstream userTypeStream;
        userTypeStream << "0x" << std::hex << std::setw(4) << std::setfill('0') << partition.user_type;
        ss << std::left << std::setw(10) << userTypeStream.str();
        ss << std::dec << std::setfill(' ');

        std::string flags;
        if (partition.keydata) flags += "K";
        if (partition.encrypt) flags += "E";
        if (partition.verify) flags += "V";
        if (partition.ro) flags += "R";

        if (flags.empty()) {
            ss << "-";
        } else {
            ss << flags;
        }

        ss << std::endl;
    }
    ss << "\nFlags: K=KeyData, E=Encrypt, V=Verify, R=Read-Only" << std::endl;
    return ss.str();
}

void OpenixPartition::dump() const {
    std::cout << dumpToString() << std::endl;
}

std::string OpenixPartition::dumpToJson() const {
    std::stringstream ss;
    ss << "{" << std::endl;
    ss << "    \"mbr_size\": " << mbrSize << "," << std::endl;
    ss << "    \"partitions\": [" << std::endl;

    // Add each partition as a JSON object
    for (size_t i = 0; i < partitions.size(); ++i) {
        const auto &partition = partitions[i];
        ss << "        {" << std::endl;
        ss << "            \"name\": \"" << partition.name << "\"," << std::endl;
        ss << "            \"size\": " << partition.size << "," << std::endl;
        ss << "            \"downloadfile\": \"" << partition.downloadfile << "\"," << std::endl;
        ss << "            \"user_type\": " << partition.user_type << "," << std::endl;
        ss << "            \"keydata\": " << (partition.keydata ? "true" : "false") << "," << std::endl;
        ss << "            \"encrypt\": " << (partition.encrypt ? "true" : "false") << "," << std::endl;
        ss << "            \"verify\": " << (partition.verify ? "true" : "false") << "," << std::endl;
        ss << "            \"ro\": " << (partition.ro ? "true" : "false") << std::endl;
        ss << "        }";

        // Add comma if not last partition
        if (i < partitions.size() - 1) {
            ss << ",";
        }
        ss << std::endl;
    }

    ss << "    ]" << std::endl;
    ss << "}" << std::endl;

    return ss.str();
}

bool OpenixPartition::parseLine(const std::string &line, Partition &partition) {
    size_t pos = 0;
    skipWhitespace(line, pos);

    if (pos >= line.size()) {
        return false;
    }

    // Parse key
    const auto key = parseIdentifier(line, pos);
    if (key.empty()) {
        return false;
    }

    skipWhitespace(line, pos);

    // Check for equal sign
    if (pos >= line.size() || line[pos] != '=') {
        return false;
    }

    pos++;
    skipWhitespace(line, pos);

    // Parse value
    if (pos >= line.size()) {
        return false;
    }

    if (key == "name") {
        partition.name = parseIdentifier(line, pos);
    } else if (key == "size") {
        partition.size = parseNumber(line, pos);
    } else if (key == "downloadfile") {
        if (pos < line.size() && line[pos] == '"') {
            partition.downloadfile = parseString(line, pos);
        } else {
            partition.downloadfile = parseIdentifier(line, pos);
        }
    } else if (key == "user_type") {
        partition.user_type = static_cast<uint32_t>(parseNumber(line, pos));
    } else if (key == "keydata") {
        partition.keydata = parseNumber(line, pos) != 0;
    } else if (key == "encrypt") {
        partition.encrypt = parseNumber(line, pos) != 0;
    } else if (key == "verify") {
        partition.verify = parseNumber(line, pos) != 0;
    } else if (key == "ro") {
        partition.ro = parseNumber(line, pos) != 0;
    }

    return true;
}

void OpenixPartition::skipWhitespace(const std::string &line, size_t &pos) {
    while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t' || line[pos] == '\r')) {
        pos++;
    }
}

std::string OpenixPartition::parseIdentifier(const std::string &line, size_t &pos) {
    std::string result;

    while (pos < line.size() && (
               std::isalnum(line[pos]) || line[pos] == '_' || line[pos] == '-' ||
               line[pos] == '.' || line[pos] == '/' || line[pos] == '\\' ||
               line[pos] == ':' || line[pos] == '#' || line[pos] == '(' || line[pos] == ')')) {
        result += line[pos];
        pos++;
    }

    return result;
}

std::string OpenixPartition::parseString(const std::string &line, size_t &pos) {
    std::string result;

    if (pos >= line.size() || line[pos] != '"') {
        return result;
    }

    pos++; // Skip opening quote

    while (pos < line.size() && line[pos] != '"') {
        // Handle escape characters
        if (line[pos] == '\\' && pos + 1 < line.size()) {
            pos++;
            result += line[pos];
        } else {
            result += line[pos];
        }
        pos++;
    }

    if (pos < line.size() && line[pos] == '"') {
        pos++; // Skip closing quote
    }

    return result;
}

uint64_t OpenixPartition::parseNumber(const std::string &line, size_t &pos) {
    uint64_t result = 0;
    bool isHex = false;

    skipWhitespace(line, pos);

    if (pos + 1 < line.size() && line[pos] == '0' && (line[pos + 1] == 'x' || line[pos + 1] == 'X')) {
        isHex = true;
        pos += 2;
    }

    if (isHex) {
        while (pos < line.size() && (
                   std::isdigit(line[pos]) ||
                   (line[pos] >= 'a' && line[pos] <= 'f') ||
                   (line[pos] >= 'A' && line[pos] <= 'F'))) {
            const auto c = line[pos];
            uint64_t digit = 0;

            if (std::isdigit(c)) {
                digit = c - '0';
            } else if (c >= 'a' && c <= 'f') {
                digit = 10 + (c - 'a');
            } else if (c >= 'A' && c <= 'F') {
                digit = 10 + (c - 'A');
            }

            result = result * 16 + digit;
            pos++;
        }
    } else {
        while (pos < line.size() && std::isdigit(line[pos])) {
            result = result * 10 + (line[pos] - '0');
            pos++;
        }
    }

    return result;
}
