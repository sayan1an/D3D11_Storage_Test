#include "d3d11_helper.h"
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <iostream>
#include <fstream>

void D3D11_Device_Resources::init(int device_index)
 {
    std::cout << "Initializing D3D11..." << std::endl;
    
    // Create DXGI factory
    IDXGIFactory* factory = nullptr;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
    if (FAILED(hr)) {
        std::cerr << "Failed to create DXGI factory. HRESULT: " << std::hex << hr << std::endl;
        device = nullptr;
        context = nullptr;
        return;
    }

    // Enumerate adapters
    IDXGIAdapter* selected_adapter = nullptr;
    UINT adapter_index = 0;
    while (factory->EnumAdapters(adapter_index, &selected_adapter) != DXGI_ERROR_NOT_FOUND) {
        DXGI_ADAPTER_DESC desc;
        selected_adapter->GetDesc(&desc);
        std::wcout << L"Device " << adapter_index << ": " << desc.Description << std::endl;
        
        if (adapter_index == device_index) {
            break;
        }
        
        selected_adapter->Release();
        selected_adapter = nullptr;
        adapter_index++;
    }

    factory->Release();
    factory = nullptr;

    if (!selected_adapter) {
        std::cerr << "Failed to find device with index " << device_index << std::endl;
        device = nullptr;
        context = nullptr;
        return;
    }

    // Create device and context
    hr = D3D11CreateDevice(
        selected_adapter,
        D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &device,
        &feature_level,
        &context
    );
    
    // Store device name
    DXGI_ADAPTER_DESC desc;
    selected_adapter->GetDesc(&desc);
    device_name = desc.Description;

    selected_adapter->Release();
    selected_adapter = nullptr;

    if (FAILED(hr)) {
        std::cerr << "Failed to create D3D11 device. HRESULT: " << std::hex << hr << std::endl;
        device = nullptr;
        context = nullptr;
        return;
    }
    std::cout << "D3D11 device created successfully. Feature Level: " << std::hex << feature_level << std::endl;
    std::wcout << L"Selected device: " << device_name << std::endl;
}

void D3D11_Device_Resources::release()
{
    if (context) context->Release();
    if (device) device->Release();

    context = nullptr;
    device = nullptr;
}

void D3D11_Compute_Shader::init_from_code_string(ID3D11Device* device, const char* shader_code, const char* entry_point, const D3D_SHADER_MACRO* defines)
{   
    if (!shader_code || !entry_point || strlen(shader_code) == 0 || strlen(entry_point) == 0) {
        std::cerr << "Shader code or entry point is empty" << std::endl;
        shader = nullptr;
        return;
    }

    ID3DBlob* shader_blob;
    ID3DBlob* error_blob;
    UINT compile_flags = D3DCOMPILE_ENABLE_STRICTNESS;
    if (debug_mode) {
        compile_flags |= D3DCOMPILE_DEBUG;
        compile_flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
    }
        
    HRESULT hr = D3DCompile(
        shader_code,                   
        strlen(shader_code),         
        nullptr,                      
        defines,                      
        D3D_COMPILE_STANDARD_FILE_INCLUDE, 
        entry_point,                     
        "cs_5_0",                     
        compile_flags,
        0,                           
        &shader_blob,                 
        &error_blob                   
    );

    if (FAILED(hr)) {
        if (error_blob) 
            std::cout << "Compile failed. " << reinterpret_cast<char*>(error_blob->GetBufferPointer()) << std::endl;
        else
            std::cout << "Compile failed. Shader path not found" << std::endl;

        shader = nullptr;
        return;
    }

    if (FAILED(device->CreateComputeShader(shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), nullptr, &shader))) {
        shader = nullptr;
        return;
    }
}

void D3D11_Compute_Shader::init_from_file(ID3D11Device* device, const char* file_path, const char* entry_point, const D3D_SHADER_MACRO* defines)
{
    std::ifstream file(file_path);
    std::string shader_code((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (shader_code.empty()) {
        std::cerr << "Failed to read shader file: " << file_path << std::endl;
        shader = nullptr;
        return;
    }
    init_from_code_string(device, shader_code.c_str(), entry_point, defines);
}

void D3D11_Compute_Shader::release()
{
    if (shader) shader->Release();
    shader = nullptr;
}