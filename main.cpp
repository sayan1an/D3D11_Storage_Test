#include "d3d11_helper.h"
#include "test.h"
#include <iostream>

bool run_compute_shader(int deviceIndex = 0)
{
    // Initialize D3D11 device and context
    D3D11_Device_Resources d3d_resources;
    d3d_resources.init(deviceIndex);
    if (d3d_resources.device == nullptr || d3d_resources.context == nullptr) {
        return false;
    }

    run_write_test(d3d_resources.device, d3d_resources.context);
    run_read_test(d3d_resources.device, d3d_resources.context);
    run_shader_compile_test(d3d_resources.device, d3d_resources.context);
    return true;
}

int main(int argc, char* argv[])
{
    int deviceIndex = 0;
    if (argc > 1) {
        deviceIndex = std::atoi(argv[1]);
    }

    if (!run_compute_shader(deviceIndex)) {
        std::cerr << "Compute shader execution failed!" << std::endl;
        return 1;
    }
    return 0;
} 