#pragma once

#include <d3d11.h>
#include <string>

struct D3D11_Device_Resources {
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    D3D_FEATURE_LEVEL feature_level;
    std::wstring device_name;
    void init(int device_index = 0);
    void release();
    ~D3D11_Device_Resources() {
        release();
    }
};

struct D3D11_Compute_Shader {
    ID3D11ComputeShader* shader = nullptr;
    void init_from_code_string(ID3D11Device* device, const char* shader_code, const char* entry_point, const D3D_SHADER_MACRO* defines = nullptr);
    void init_from_file(ID3D11Device* device, const char* file_path, const char* entry_point, const D3D_SHADER_MACRO* defines = nullptr);
    void release();
    ~D3D11_Compute_Shader() {
        release();
    }
private:
    bool debug_mode = false; // Sets flag D3DCOMPILE_DEBUG and D3DCOMPILE_SKIP_OPTIMIZATION if true
};