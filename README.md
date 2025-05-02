# D3D11 Storage Test

This project demonstrates the use of D3D11 compute shaders for texture-array operations. It creates various types of texture-arrays, performs a compute operation on it, and verifies the results by bi-directionally transferring data between host and device.

## Prerequisites

- Windows 10 or later
- Visual Studio 2019 or later
- CMake 3.10 or later
- DirectX 11 compatible graphics card

## Build Steps

### Debug Build
```bash
# Create build directory and configure
cmake -B build

# Build the project
cmake --build build
```

### Release Build
```bash
# Create build directory and configure
cmake -B build

# Build the project in Release mode
cmake --build build --config Release
```

## Running the Program

The program can be run with an optional device index parameter to select which GPU to use:

```bash
# Run using default device (device 0)
./build/Debug/D3D11_Storage_Test.exe

# Run using a specific device
./build/Debug/D3D11_Storage_Test.exe 1  # Use device 1
```

When run without arguments, it will:
1. List all available graphics devices
2. Use the first device (index 0) by default
3. Execute compute shaders and perform complex operations, fetching and writing values to and from texture-arrays
4. Verify the results

## Notes

- The file in 'shaders' directory is automatically copied to the build directory during the build process
- The program requires a DirectX 11 compatible graphics card with compute shader support
- The compute shader is compiled at runtime using the D3DCompiler 