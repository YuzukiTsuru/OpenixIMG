# OpenixIMG

A comprehensive toolset for handling Allwinner IMAGEWTY format firmware images.

## Overview

OpenixIMG is a powerful command-line utility designed for creating, modifying, decrypting, and analyzing Allwinner IMAGEWTY firmware image files. It provides a complete solution for firmware developers and enthusiasts working with Allwinner-based devices.

## Features

- **Image Unpacking**: Extract files from firmware images in multiple output formats
- **Partition Table Analysis**: Extract and display partition table information from images
- **Advanced Error Handling**: Exception-based error management for better error propagation and handling
- **Centralized Logging System**: OpenixUtils class providing controlled verbose output management
- **Cross-platform**: CMake-based build system for compatibility across operating systems (Windows, Linux, macOS)
- **Extensible Architecture**: Modular design for easy integration and extension

## Building OpenixIMG

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2019+)
- CMake 3.15 or newer
- Git (for cloning submodules)

### Build Steps

1. Clone the repository with submodules:
   ```bash
   git clone --recurse-submodules https://github.com/YuzukiTsuru/OpenixIMG.git
   cd OpenixIMG
   ```

2. Create build directory:
   ```bash
   mkdir build
   cd build
   ```

3. Configure the project:
   ```bash
   cmake ..
   ```

4. Build the project:
   ```bash
   cmake --build .
   ```

5. For Windows with Debug configuration:
   ```bash
   cmake --build . --config Debug
   ```

## Usage

OpenixIMG provides two main operations: `unpack` and `partition`.

### Basic Syntax
```
OpenixIMG <operation> -i <input> -o <output> [options]
```

### Operations

- **unpack**: Extract files from an image file
- **partition**: Output partition table from an image file

### Options

- `-i <path>`: Input file
- `-o <path>`: Output file or directory
- `-v, --verbose`: Show detailed information
- `--format <fmt>`: Output format for unpack operation (unimg or imgrepacker)
- `-h, --help`: Show help message

### Examples

#### Extract files from an image file
```bash
# Extract in imgrepacker format
OpenixIMG unpack -i firmware.img -o ./extracted_files --format imgrepacker

# Extract in unimg format
OpenixIMG unpack -i firmware.img -o ./extracted_files --format unimg

# Extract with verbose output
OpenixIMG unpack -i firmware.img -o ./extracted_files --format imgrepacker -v
```

#### Display partition table information
```bash
# Display on screen
OpenixIMG partition -i firmware.img

# Save to text file
OpenixIMG partition -i firmware.img -o partition_table.txt

# Display with verbose details
OpenixIMG partition -i firmware.img -v
```

## Project Structure

```
├── app/               # Application source code
│   ├── CMakeLists.txt # CMake configuration for the application
│   └── OpenixIMG.cpp  # Main application implementation with command-line interface
├── includes/          # Public header files
│   ├── OpenixCFG.hpp          # Configuration file parser interface
│   ├── OpenixIMGFile.hpp      # IMG file handler interface
│   ├── OpenixIMGWTY.hpp       # IMAGEWTY format definitions and structures
│   ├── OpenixPacker.hpp       # Image packing/unpacking functionality interface
│   ├── OpenixPartition.hpp    # Partition table parser interface
│   └── OpenixUtils.hpp        # Utility class with logging and common functions
├── lib/               # External libraries
│   ├── rc6/           # RC6 encryption algorithm implementation
│   └── twofish/       # Twofish encryption algorithm implementation
├── src/               # Library source code
│   ├── CMakeLists.txt         # CMake configuration for the library
│   ├── OpenixCFG.cpp          # Configuration parser implementation
│   ├── OpenixIMGFile.cpp      # IMG file handler implementation
│   ├── OpenixIMGWTY.cpp       # IMAGEWTY format implementation
│   ├── OpenixPacker.cpp       # Packer implementation
│   ├── OpenixPartition.cpp    # Partition parser implementation
│   └── OpenixUtils.cpp        # Utility class implementation
├── test/              # Test files
│   ├── CMakeLists.txt         # CMake configuration for tests
│   ├── OpenixCFGTest.cpp      # Configuration parser tests
│   ├── OpenixPartitionTest.cpp # Partition parser tests
│   └── files/                 # Test data files
├── CMakeLists.txt     # Main CMake configuration file
├── LICENSE            # MIT License file
├── README.md          # Project documentation
└── .gitmodules        # Git submodules configuration
```

## Core Components

### OpenixPacker
Responsible for unpacking image files into directories. It supports different output formats and uses exception-based error handling for better error propagation.

### OpenixIMGFile
Handles the core operations for working with IMG files, including loading, saving, and manipulating image data. It interfaces with the encryption algorithms and provides methods for reading and writing image structures.

### OpenixPartition
Parses and manages partition table information from `sys_partition.fex` files, providing methods to access partition details and export them in various formats. It supports both parsing from files and from in-memory data.

### OpenixCFG
Implements a parser for DragonEx image configuration files, allowing access to configuration variables and groups. It supports reading from files and memory buffers.

### OpenixIMGWTY
Defines the structure of the IMAGEWTY format, including image headers, file headers, and associated metadata. It provides the low-level structures used throughout the library.

### OpenixUtils
A utility class providing centralized logging functionality with configurable verbosity. It replaces individual verbose flags in components, offering a consistent way to control output across the entire library.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgements

- This project uses the [RC6](lib/rc6) and [Twofish](lib/twofish) encryption algorithms
- Special thanks to the [Allwinner community](https://linux-sunxi.org/LiveSuit_images)
