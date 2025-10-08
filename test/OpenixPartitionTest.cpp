#include <iostream>
#include <iomanip>

#include "OpenixPartition.hpp"

int main() {
    // Create an OpenixPartition instance
    OpenixIMG::OpenixPartition partitionParser;

    // Load the partition table file
    const std::string partitionFilePath = "d:/Github/OpenixIMG/test/files/sys_partition.fex";
    if (!partitionParser.parseFromFile(partitionFilePath)) {
        std::cerr << "Failed to load partition table file!" << std::endl;
        return 1;
    }

    std::cout << "Partition table loaded successfully from " << partitionFilePath << std::endl;

    // Test getting MBR size
    const uint32_t mbrSize = partitionParser.getMbrSize();
    std::cout << "MBR size: " << mbrSize << " KB" << std::endl;

    // Test getting all partitions
    const auto &partitions = partitionParser.getPartitions();
    std::cout << "Total partitions: " << partitions.size() << std::endl;

    // Test dumpToString method
    std::cout << "\nUsing OpenixPartition::dumpToString() method to display partition table information:\n" <<
            std::endl;
    std::cout << partitionParser.dumpToString() << std::endl;

    // Test dump method
    std::cout << "\nUsing OpenixPartition::dump() method to display partition table information:\n" << std::endl;
    partitionParser.dump();

    // Test dumpToJson method
    std::cout << "\nUsing OpenixPartition::dumpToJson() method to get JSON formatted partition table:\n" << std::endl;
    std::string jsonOutput = partitionParser.dumpToJson();
    std::cout << jsonOutput << std::endl;

    // Test getting partition by name
    const std::string testPartitionName = "boot";
    if (const auto partition = partitionParser.getPartitionByName(testPartitionName)) {
        std::cout << "\nFound partition: " << testPartitionName << std::endl;
        std::cout << "  Size: " << partition->size << " sectors" << std::endl;
        std::cout << "  Download file: " << partition->downloadfile << std::endl;
        std::cout << "  User type: 0x" << std::hex << partition->user_type << std::dec << std::endl;
    } else {
        std::cout << "\nPartition " << testPartitionName << " not found." << std::endl;
    }

    // Test checking if partition name exists
    const std::string existingName = "rootfs";
    const std::string nonExistingName = "non_existing_partition";

    std::cout << "\nCheck partition name existence:" << std::endl;
    std::cout << "- " << existingName << " exists: " << (partitionParser.isPartitionNameExists(existingName)
                                                             ? "Yes"
                                                             : "No") << std::endl;
    std::cout << "- " << nonExistingName << " exists: " << (partitionParser.isPartitionNameExists(nonExistingName)
                                                                ? "Yes"
                                                                : "No") << std::endl;

    // Test specific partition properties from the sample file
    if (const auto bootResourcePartition = partitionParser.getPartitionByName("boot-resource")) {
        std::cout << "\nVerifying boot-resource partition properties:" << std::endl;
        std::cout << "  Expected size: 256, Actual size: " << bootResourcePartition->size << std::endl;
        std::cout << "  Expected downloadfile: boot-resource.fex, Actual: " << bootResourcePartition->downloadfile <<
                std::endl;
        std::cout << "  Expected user_type: 0x8000, Actual: 0x" << std::hex << bootResourcePartition->user_type <<
                std::dec << std::endl;
    }

    std::cout << "\nOpenixPartition test completed." << std::endl;
    return 0;
}
