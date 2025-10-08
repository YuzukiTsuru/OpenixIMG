#include <iostream>
#include <string>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <fstream>
#include <iomanip>

#include "OpenixPacker.hpp"
#include "OpenixPartition.hpp"


// Define program version information
#define VERSION "1.0.0"

// Command line argument parsing function
bool parseArguments(const int argc, char *argv[], std::string &operation,
                    std::string &input, std::string &output,
                    bool &verbose, bool &noEncrypt,
                    OpenixIMG::OutputFormat &outputFormat) {
    if (argc < 2) {
        return false;
    }

    // Standard operation handling
    operation = argv[1];

    // Convert to lowercase for case-insensitive comparison
    std::transform(operation.begin(), operation.end(), operation.begin(),
                   [](const unsigned char c) { return std::tolower(c); });

    // Check if it's a valid operation
    if (operation != "pack" && operation != "decrypt" && operation != "unpack" && operation != "partition") {
        return false;
    }

    // Parse remaining arguments
    for (int i = 2; i < argc; ++i) {
        if (std::string arg = argv[i]; arg == "-i" && i + 1 < argc) {
            input = argv[++i];
        } else if (arg == "-o" && i + 1 < argc) {
            output = argv[++i];
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "--no-encrypt") {
            noEncrypt = true;
        } else if (arg == "--format" && i + 1 < argc) {
            if (std::string formatArg = argv[++i]; formatArg == "unimg") {
                outputFormat = OpenixIMG::OutputFormat::UNIMG;
            } else if (formatArg == "imgrepacker") {
                outputFormat = OpenixIMG::OutputFormat::IMGREPACKER;
            } else {
                std::cerr << "Warning: Unknown output format: " << formatArg << ", using default (unimg)" << std::endl;
            }
        } else if (arg == "--help" || arg == "-h") {
            return false;
        }
    }

    // Validate required parameters (output is optional for partition operation)
    if (input.empty()) {
        return false;
    }

    return true;
}

// Display help information
void showHelp(const char *programName) {
    std::cout << "OpenixIMG v" << VERSION << std::endl;
    std::cout << "Usage: " << programName << " <operation> -i <input> -o <output> [options]" << std::endl;
    std::cout << "       " << programName << " partition -i <image_file> [-o <output_file>]" << std::endl;
    std::cout << std::endl;
    std::cout << "Operations:" << std::endl;
    std::cout << "  pack       Pack a directory into an image file" << std::endl;
    std::cout << "  decrypt    Decrypt an encrypted image file" << std::endl;
    std::cout << "  unpack     Extract files from an image file" << std::endl;
    std::cout << "  partition  Output partition table from an image file" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -i <path>       Input file or directory" << std::endl;
    std::cout << "  -o <path>       Output file or directory" << std::endl;
    std::cout << "  -v, --verbose   Show detailed information" << std::endl;
    std::cout << "  --no-encrypt    Disable encryption (pack operation only)" << std::endl;
    std::cout << "  --format <fmt>  Output format for unpack operation (unimg or imgrepacker)" << std::endl;
    std::cout << "  -h, --help      Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " pack -i ./firmware_dir -o firmware.img" << std::endl;
    std::cout << "  " << programName << " decrypt -i encrypted.img -o decrypted.img" << std::endl;
    std::cout << "  " << programName << " unpack -i firmware.img -o ./extracted_files --format imgrepacker" <<
            std::endl;
    std::cout << "  " << programName << " partition -i firmware.img" << std::endl;
    std::cout << "  " << programName << " partition -i firmware.img -o partition_table.txt" << std::endl;
}

int main(const int argc, char *argv[]) {
    std::string operation;
    std::string input;
    std::string output;
    bool verbose = false;
    bool noEncrypt = false;
    auto outputFormat = OpenixIMG::OutputFormat::UNIMG;

    // Parse command line arguments
    if (!parseArguments(argc, argv, operation, input, output, verbose, noEncrypt, outputFormat)) {
        showHelp(argv[0]);
        return 1;
    }

    try {
        // Create ImagePacker instance
        OpenixIMG::OpenixPacker packer;
        packer.setVerbose(verbose);

        if (verbose) {
            std::cout << "OpenixIMG v" << VERSION << " started" << std::endl;
            std::cout << "Operation: " << operation << std::endl;
            std::cout << "Input: " << input << std::endl;
            std::cout << "Output: " << output << std::endl;
        }

        bool success = false;

        // Execute the specified operation
        if (operation == "pack") {
            if (verbose) {
                std::cout << "Packing directory into image file..." << std::endl;
            }

            packer.setEncryptionEnabled(!noEncrypt);
            if (verbose && noEncrypt) {
                std::cout << "Encryption: disabled" << std::endl;
            }

            success = packer.packImage(input, output);
        } else if (operation == "decrypt") {
            if (verbose) {
                std::cout << "Decrypting image file..." << std::endl;
            }

            if (!packer.loadImage(input)) {
                std::cerr << "Failed to load image file!" << std::endl;
                return 1;
            }
            success = packer.decryptImage(output);
        } else if (operation == "unpack") {
            if (verbose) {
                std::cout << "Unpacking image file..." << std::endl;
                std::cout << "Output format: " <<
                        (outputFormat == OpenixIMG::OutputFormat::UNIMG ? "unimg" : "imgrepacker") << std::endl;
            }

            if (!packer.loadImage(input)) {
                std::cerr << "Failed to load image file!" << std::endl;
                return 1;
            }
            packer.setOutputFormat(outputFormat);
            success = packer.unpackImage(output);
        } else if (operation == "partition") {
            // Handle partition operation: only read partition data
            std::cout << "Reading sys_partition.fex from image..." << std::endl;

            if (!packer.loadImage(input)) {
                std::cerr << "Failed to load image file!" << std::endl;
                return 1;
            }

            // Directly get sys_partition.fex file data
            auto fileData = packer.getFileDataByFilename("sys_partition.fex");
            if (!fileData) {
                std::cerr << "Failed to find sys_partition.fex in the image!" << std::endl;
                return 1;
            }

            std::cout << "Found sys_partition.fex. Parsing partition table directly from memory..." << std::endl;

            // Parse partition table directly from memory data
            if (OpenixIMG::OpenixPartition partitionParser; partitionParser.parseFromData(
                fileData->data(), fileData->size())) {
                // Use dumpToString method to get formatted partition table information
                std::string partitionInfo = partitionParser.dumpToString();

                // Output results to console or file
                if (!output.empty()) {
                    // Output to file
                    if (std::ofstream outFile(output, std::ios::out | std::ios::binary); outFile.is_open()) {
                        outFile << partitionInfo;
                        outFile.close();
                        std::cout << "Partition table information has been written to " << output << std::endl;
                    } else {
                        std::cerr << "Failed to open output file: " << output << std::endl;
                        std::cout << partitionInfo; // still output to console
                    }
                } else {
                    // Output to console
                    std::cout << partitionInfo;
                }
            } else {
                std::cerr << "Failed to parse sys_partition.fex!" << std::endl;
                return 1;
            }

            return 0;
        }
        // Default return operation success/failure status
        if (success) {
            if (verbose) {
                std::cout << "Operation completed successfully!" << std::endl;
            }
            return 0;
        }
        std::cerr << "Operation failed!" << std::endl;
        return 1;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred!" << std::endl;
        return 1;
    }
}
