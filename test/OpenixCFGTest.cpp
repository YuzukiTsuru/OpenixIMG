#include <iostream>

#include "OpenixCFG.hpp"

int main() {
    // Create a CFGParser instance
    OpenixCFG parser;
    
    // Load the configuration file
    if (!parser.loadFromFile("d:/Github/OpenixIMG/test/files/image.cfg")) {
        std::cerr << "Failed to load config file!" << std::endl;
        return 1;
    }
    
    std::cout << "Config file loaded successfully." << std::endl;
    
    // Test finding a group
    if (std::shared_ptr<Group> mainTypeGroup = parser.findGroup("MAIN_TYPE")) {
        std::cout << "Found group: MAIN_TYPE" << std::endl;
        std::cout << "Number of variables in MAIN_TYPE: " << parser.countVariables("MAIN_TYPE") << std::endl;
    } else {
        std::cout << "Group MAIN_TYPE not found." << std::endl;
    }
    
    // Test retrieving a string value
    if (const auto commonType = parser.getString("ITEM_COMMON")) {
        std::cout << "ITEM_COMMON = " << *commonType << std::endl;
    } else {
        std::cout << "ITEM_COMMON not found." << std::endl;
    }
    
    // Test retrieving a numeric value
    if (const auto version = parser.getNumber("version", "IMAGE_CFG")) {
        std::cout << "IMAGE_CFG.version = 0x" << std::hex << *version << std::dec << std::endl;
    } else {
        std::cout << "IMAGE_CFG.version not found." << std::endl;
    }
    
    // Print the entire configuration (for debugging)
    parser.dump();
    
    std::cout << "CFGParser test completed." << std::endl;
    return 0;
}