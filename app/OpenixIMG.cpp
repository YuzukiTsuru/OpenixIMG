#include <iostream>
#include <string>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <fstream>
#include <iomanip>

#include "OpenixPacker.hpp"
#include "OpenixPartition.hpp"


// 定义程序版本信息
#define VERSION "1.0.0"

// 命令行参数处理函数
bool parseArguments(const int argc, char *argv[], std::string &operation,
                    std::string &input, std::string &output,
                    bool &verbose, bool &noEncrypt,
                    OpenixIMG::OutputFormat &outputFormat) {
    if (argc < 2) {
        return false;
    }

    // 标准操作处理
    operation = argv[1];

    // 转换为小写以进行大小写不敏感比较
    std::transform(operation.begin(), operation.end(), operation.begin(),
                   [](const unsigned char c) { return std::tolower(c); });

    // 检查是否是有效的操作
    if (operation != "pack" && operation != "decrypt" && operation != "unpack" && operation != "partition") {
        return false;
    }

    // 解析剩余参数
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

    // 验证必要参数（partition操作的output是可选的）
    if (input.empty()) {
        return false;
    }

    return true;
}

// 显示帮助信息
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

    // 解析命令行参数
    if (!parseArguments(argc, argv, operation, input, output, verbose, noEncrypt, outputFormat)) {
        showHelp(argv[0]);
        return 1;
    }

    try {
        // 创建ImagePacker实例
        OpenixIMG::OpenixPacker packer;
        packer.setVerbose(verbose);

        if (verbose) {
            std::cout << "OpenixIMG v" << VERSION << " started" << std::endl;
            std::cout << "Operation: " << operation << std::endl;
            std::cout << "Input: " << input << std::endl;
            std::cout << "Output: " << output << std::endl;
        }

        bool success = false;

        // 执行指定的操作
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
            // 处理partition操作：只读取partition数据
            std::cout << "Reading sys_partition.fex from image..." << std::endl;

            if (!packer.loadImage(input)) {
                std::cerr << "Failed to load image file!" << std::endl;
                return 1;
            }

            // 直接获取sys_partition.fex文件数据
            auto fileData = packer.getFileDataByFilename("sys_partition.fex");
            if (!fileData) {
                std::cerr << "Failed to find sys_partition.fex in the image!" << std::endl;
                return 1;
            }

            std::cout << "Found sys_partition.fex. Parsing partition table directly from memory..." << std::endl;

            // 直接从内存数据解析分区表
            if (OpenixIMG::OpenixPartition partitionParser; partitionParser.parseFromData(
                fileData->data(), fileData->size())) {
                // 使用dumpToString方法获取格式化的分区表信息
                std::string partitionInfo = partitionParser.dumpToString();

                // 输出结果到控制台或文件
                if (!output.empty()) {
                    // 输出到文件
                    if (std::ofstream outFile(output, std::ios::out | std::ios::binary); outFile.is_open()) {
                        outFile << partitionInfo;
                        outFile.close();
                        std::cout << "Partition table information has been written to " << output << std::endl;
                    } else {
                        std::cerr << "Failed to open output file: " << output << std::endl;
                        std::cout << partitionInfo; // 仍然输出到控制台
                    }
                } else {
                    // 输出到控制台
                    std::cout << partitionInfo;
                }
            } else {
                std::cerr << "Failed to parse sys_partition.fex!" << std::endl;
                return 1;
            }

            return 0;
        }
        // 默认返回操作成功/失败状态
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
