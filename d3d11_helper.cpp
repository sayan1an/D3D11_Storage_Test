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

void D3D11_Constant_Buffer::init(ID3D11Device* device, size_t bytes)
{
    if (bytes % 16 != 0) {
        std::cerr << "Constant buffer size must be a multiple of 16." << std::endl;
        p_buffer = nullptr;
        return;
    }

    blob_size = bytes;
    
    D3D11_BUFFER_DESC desc;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth = (UINT)bytes;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    
    if (FAILED(device->CreateBuffer(&desc, nullptr, &p_buffer))) {
        std::cerr << "Failed to create constant buffer." << std::endl;
        p_buffer = nullptr;
        return;
    }
}

void D3D11_Constant_Buffer::to_gpu(ID3D11DeviceContext* context, const void* data)
{
    D3D11_MAPPED_SUBRESOURCE mapped_resource;
    if (FAILED(context->Map(p_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource))) {
        std::cerr << "Failed to map constant buffer." << std::endl;
        return;
    }
    memcpy(mapped_resource.pData, data, blob_size);
    context->Unmap(p_buffer, 0);
}

void D3D11_Constant_Buffer::release()
{
    if (p_buffer) p_buffer->Release();
    p_buffer = nullptr;
}

void D3D11_Performance_Counter::init(ID3D11Device* device)
{
    D3D11_QUERY_DESC query_desc = {};
    query_desc.Query = D3D11_QUERY_TIMESTAMP;
        
    if (FAILED(device->CreateQuery(&query_desc, &p_start_query))) {
        std::cerr << "Failed to create start query." << std::endl;
        p_start_query = nullptr;
        p_end_query = nullptr;
        p_disjoint_query = nullptr;
        return;
    }
    if (FAILED(device->CreateQuery(&query_desc, &p_end_query))) {
        std::cerr << "Failed to create end query." << std::endl;
        p_start_query = nullptr;
        p_end_query = nullptr;
        p_disjoint_query = nullptr;
        return;
    }

    // Create disjoint query to check if timestamps are valid
    D3D11_QUERY_DESC disjoint_desc = {};
    disjoint_desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    if (FAILED(device->CreateQuery(&disjoint_desc, &p_disjoint_query))) {
        std::cerr << "Failed to create disjoint query." << std::endl;
        p_start_query = nullptr;
        p_end_query = nullptr;
        p_disjoint_query = nullptr;
        return;
    }
}

void D3D11_Performance_Counter::counter_start(ID3D11DeviceContext* context)
{   
    if (p_start_query == nullptr || p_end_query == nullptr || p_disjoint_query == nullptr) {
        std::cerr << "Performance counter not initialized, run init() first." << std::endl;
        return;
    }

    if (performance_counter_initialized) {
        std::cerr << "Performance counter already initialized, run counter_stop() first." << std::endl;
        return;
    }
    
    context->Flush();
    context->Begin(p_disjoint_query);
    context->End(p_start_query);
    performance_counter_initialized = true;
}

double D3D11_Performance_Counter::counter_stop(ID3D11DeviceContext* context)
{
    if (!performance_counter_initialized) {
        std::cerr << "Performance counter not initialized, run counter_start() first." << std::endl;
        return 0.0;
    }

    context->Flush();
    context->End(p_end_query);
    context->End(p_disjoint_query);
    performance_counter_initialized = false;
    
    // Get results
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint_data = {};
    UINT64 start_time = 0;
    UINT64 end_time = 0;

    // Wait for results
    while (context->GetData(p_disjoint_query, &disjoint_data, sizeof(disjoint_data), 0) == S_FALSE) {}
    while (context->GetData(p_start_query, &start_time, sizeof(start_time), 0) == S_FALSE) {}
    while (context->GetData(p_end_query, &end_time, sizeof(end_time), 0) == S_FALSE) {}

    // Calculate time in milliseconds
    if (!disjoint_data.Disjoint) {
        return (end_time - start_time) * 1000.0 / disjoint_data.Frequency; // Return time in milliseconds
    }
    else {  
        std::cerr << "Timestamp discontinuity detected, results may be invalid." << std::endl;
        return 0.0;
    }

    return 0.0;
}

void D3D11_Performance_Counter::release()
{
    if (p_start_query) p_start_query->Release();
    if (p_end_query) p_end_query->Release();
    if (p_disjoint_query) p_disjoint_query->Release();
    p_start_query = nullptr;
    p_end_query = nullptr;
    p_disjoint_query = nullptr;
}



