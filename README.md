# D3D11 Storage Test

This project demonstrates the use of D3D11 compute shaders for array operations. It creates a buffer, performs a compute operation on it, and verifies the results.

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
3. Execute a compute shader that doubles each value in an array
4. Verify the results

## Project Structure

- `main.cpp` - Main application code
- `ArraySum.hlsl` - Compute shader source
- `Common.h` - Common definitions and constants

## Notes

- The shader file `ArraySum.hlsl` is automatically copied to the build directory during the build process
- The program requires a DirectX 11 compatible graphics card with compute shader support
- The compute shader is compiled at runtime using the D3DCompiler 