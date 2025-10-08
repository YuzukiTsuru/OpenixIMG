# OpenixIMG

A comprehensive toolset for handling Allwinner IMAGEWTY format firmware images.

## Overview

OpenixIMG is a powerful command-line utility designed for creating, modifying, decrypting, and analyzing Allwinner IMAGEWTY firmware image files. It provides a complete solution for firmware developers and enthusiasts working with Allwinner-based devices.

## Features

- **Image Packing**: Convert directories into IMAGEWTY format firmware images
- **Image Unpacking**: Extract files from firmware images
- **Image Decryption**: Decrypt encrypted firmware images
- **Partition Table Analysis**: Extract and display partition table information from images
- **Encryption Support**: Built-in RC6 and Twofish encryption algorithms
- **Multiple Output Formats**: Support for different output formats during unpacking
- **Cross-platform**: CMake-based build system for compatibility across operating systems

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

OpenixIMG provides four main operations: `pack`, `decrypt`, `unpack`, and `partition`.

### Basic Syntax
```
OpenixIMG <operation> -i <input> -o <output> [options]
```

### Operations

- **pack**: Pack a directory into an image file
- **decrypt**: Decrypt an encrypted image file
- **unpack**: Extract files from an image file
- **partition**: Output partition table from an image file

### Options

- `-i <path>`: Input file or directory
- `-o <path>`: Output file or directory
- `-v, --verbose`: Show detailed information
- `--no-encrypt`: Disable encryption (pack operation only)
- `--format <fmt>`: Output format for unpack operation (unimg or imgrepacker)
- `-h, --help`: Show help message

### Examples

#### Pack a directory into an image file
```bash
OpenixIMG pack -i ./firmware_dir -o firmware.img
```

#### Decrypt an encrypted image file
```bash
OpenixIMG decrypt -i encrypted.img -o decrypted.img
```

#### Extract files from an image file
```bash
OpenixIMG unpack -i firmware.img -o ./extracted_files --format imgrepacker
```

#### Display partition table information
```bash
OpenixIMG partition -i firmware.img
```

#### Save partition table to a file
```bash
OpenixIMG partition -i firmware.img -o partition_table.txt
```

## Project Structure

```
├── app/               # Application source code
│   ├── CMakeLists.txt # CMake configuration for the application
│   └── OpenixIMG.cpp  # Main application implementation
├── includes/          # Public header files
│   ├── OpenixCFG.hpp          # Configuration file parser
│   ├── OpenixIMGWTY.hpp       # IMAGEWTY format definitions
│   ├── OpenixPacker.hpp       # Image packing/unpacking functionality
│   └── OpenixPartition.hpp    # Partition table parser
├── lib/               # External libraries
│   ├── rc6/           # RC6 encryption algorithm
│   └── twofish/       # Twofish encryption algorithm
├── src/               # Library source code
│   ├── OpenixCFG.cpp          # Configuration parser implementation
│   ├── OpenixIMGWTY.cpp       # IMAGEWTY format implementation
│   ├── OpenixPacker.cpp       # Packer implementation
│   └── OpenixPartition.cpp    # Partition parser implementation
└── test/              # Test files
    ├── OpenixCFGTest.cpp      # Configuration parser tests
    ├── OpenixPartitionTest.cpp # Partition parser tests
    └── files/                 # Test data files
```

## Core Components

### OpenixPacker
Responsible for packing directories into image files, unpacking image files into directories, and decrypting encrypted image files. It supports different output formats and encryption options.

### OpenixPartition
Parses and manages partition table information from `sys_partition.fex` files, providing methods to access partition details and export them in various formats.

### OpenixCFG
Implements a parser for DragonEx image configuration files, allowing access to configuration variables and groups.

### OpenixIMGWTY
Defines the structure of the IMAGEWTY format, including image headers, file headers, and associated metadata.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgements

- This project uses the [RC6](lib/rc6) and [Twofish](lib/twofish) encryption algorithms
- Special thanks to the [Allwinner community](https://linux-sunxi.org/LiveSuit_images)
