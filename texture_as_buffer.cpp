#include "texture_as_buffer.h"
#include <iostream>

void Texture_As_Buffer::init(ID3D11Device* device, size_t __channels, size_t __height, size_t __width, DXGI_FORMAT format)
{   
    if (__channels * __height * __width == 0) {
        std::cout << "Failed to initialize. Channels, Height, Width must be non-zero." << std::endl;
        p_texture = nullptr;
        p_texture_uav = nullptr;
        p_texture_srv = nullptr;
        return;
    }

    height = __height;
    width = __width;
    channels = __channels;

    switch (format) {
        case DXGI_FORMAT_R8_UNORM:
            element_size = 1;
            break;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            element_size = 4;
            break;
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            element_size = 4;
            break;
        case DXGI_FORMAT_R16_FLOAT:
            element_size = 2;
            break;
        case DXGI_FORMAT_R16G16_FLOAT:
            element_size = 4;
            break;
        case DXGI_FORMAT_R32_FLOAT:
            element_size = 4;
            break;
        default:
            std::cout << "Failed to initialize. Unrecoginzed format." << std::endl;
            p_texture = nullptr;
            p_texture_uav = nullptr;
            p_texture_srv = nullptr;
            return;
    }
    
    // Create the texture
    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = (UINT)width;
    tex_desc.Height = (UINT)height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = (UINT)channels;
    tex_desc.Format = format;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    tex_desc.CPUAccessFlags = 0;
    
    if (FAILED(device->CreateTexture2D(&tex_desc, nullptr, &p_texture))) {
        std::cout << "Failed to create texture." << std::endl;
        p_texture = nullptr;
        p_texture_uav = nullptr;
        p_texture_srv = nullptr;
        return;
    }

    D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    uav_desc.Format = format;
    uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
    uav_desc.Texture2DArray.MipSlice = 0;
    uav_desc.Texture2DArray.FirstArraySlice = 0;
    uav_desc.Texture2DArray.ArraySize = (UINT)channels;
                            
    if (FAILED(device->CreateUnorderedAccessView(p_texture, &uav_desc, &p_texture_uav))) {
        std::cout << "Failed to create texture UAV." << std::endl;
        p_texture = nullptr;
        p_texture_uav = nullptr;
        p_texture_srv = nullptr;
        return;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srv_desc.Texture2DArray.MipLevels = 1;
    srv_desc.Texture2DArray.FirstArraySlice = 0;
    srv_desc.Texture2DArray.ArraySize = (UINT)channels;
    
    if (FAILED(device->CreateShaderResourceView(p_texture, &srv_desc, &p_texture_srv))) {
        std::cout << "Failed to create texture SRV." << std::endl;
        p_texture = nullptr;
        p_texture_uav = nullptr;
        p_texture_srv = nullptr;
        return;
    }

    std::cout << "Created texture of shape: " << print_shape() << std::endl;
}

void Texture_As_Buffer::init_staging(ID3D11Device* device)
{
    if (p_texture == nullptr) {
        std::cout << "Cannot create staging texture, init() texture first." << std::endl;
        p_texture_staging = nullptr;
        return;
    }

    // Create the staging texture
    D3D11_TEXTURE2D_DESC staging_desc = {};
    p_texture->GetDesc(&staging_desc);
    staging_desc.Usage = D3D11_USAGE_STAGING;
    staging_desc.BindFlags = 0;
    staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
            
    if (FAILED(device->CreateTexture2D(&staging_desc, nullptr, &p_texture_staging))) {
        std::cout << "Failed to create staging buffer." << std::endl;
        p_texture_staging = nullptr;
        return;
    }
    
    data = new unsigned char[channels * height * width * element_size];
}

void* Texture_As_Buffer::to_cpu(ID3D11DeviceContext* context)
{   
    if (p_texture_staging == nullptr) {
        std::cout << "Cannot fetch data to cpu, init_staging() first." << std::endl;
        return nullptr;
    }

    context->Flush();
    context->CopyResource(p_texture_staging, p_texture);
    context->Flush();
    
    // Get the texture description to determine size
    D3D11_TEXTURE2D_DESC desc;
    p_texture_staging->GetDesc(&desc);

    // Map the staging buffer
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (FAILED(context->Map(p_texture_staging, 0, D3D11_MAP_READ, 0, &mapped))) {
        std::cout << "Cannot fetch data to cpu, failed to map staging buffer." << std::endl;
        return nullptr;
    }
    
    // Copy data row by row (handling pitch)
    const unsigned char* src = static_cast<const unsigned char*>(mapped.pData);
    const size_t src_row_pitch = mapped.RowPitch;
    unsigned char* dst = (unsigned char*)data;
    const size_t dst_row_pitch = desc.Width * element_size;
            
    for (UINT row = 0; row < desc.Height * channels; row++)
        memcpy(dst + row * dst_row_pitch, src + row * src_row_pitch, dst_row_pitch);
            
    // Unmap the staging buffer
    context->Unmap(p_texture_staging, 0);
    
    return data;
}

void Texture_As_Buffer::to_gpu(ID3D11DeviceContext* context, void *data)
{
    if (p_texture_staging == nullptr) {
        std::cout << "Cannot push data to gpu, init_staging() first." << std::endl;
        return;
    }

    if (data == nullptr) {
        std::cout << "Cannot push data to gpu, data is nullptr." << std::endl;
        return;
    }
    
    // Get the texture description to determine size
    D3D11_TEXTURE2D_DESC desc;
    p_texture_staging->GetDesc(&desc);

    // Map the staging buffer
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (FAILED(context->Map(p_texture_staging, 0, D3D11_MAP_READ, 0, &mapped))) {
        std::cout << "Cannot fetch data to cpu, failed to map staging buffer." << std::endl;
        return;
    }
    
    // Copy data row by row (handling pitch)
    unsigned char* dst = static_cast<unsigned char*>(mapped.pData);
    const size_t dst_row_pitch = mapped.RowPitch;
    const unsigned char* src = (const unsigned char*)data;
    const size_t src_row_pitch = desc.Width * element_size;
    
    for (UINT row = 0; row < desc.Height * channels; row++)
        memcpy(dst + row * dst_row_pitch, src + row * src_row_pitch, src_row_pitch);
            
    // Unmap the staging buffer
    context->Unmap(p_texture_staging, 0);

    context->Flush();
    context->CopyResource(p_texture, p_texture_staging);
    context->Flush();
}

void Texture_As_Buffer::to_gpu(ID3D11DeviceContext* context, unsigned char clear_val)
{
    // Prepare the initialization data
    unsigned char *init_data = new unsigned char[channels * height * width * element_size];
    for (int i = 0; i < channels * height * width * element_size; i++)
        init_data[i] = clear_val;

    to_gpu(context, init_data);
    delete[] init_data;
}

void Texture_As_Buffer::to_gpu(ID3D11DeviceContext* context, unsigned int clear_val)
{
    // Prepare the initialization data
    unsigned int *init_data = new unsigned int[(channels * height * width * element_size + 3) / 4];
    for (int i = 0; i < (channels * height * width * element_size + 3) / 4; i++)
        init_data[i] = clear_val;

    to_gpu(context, init_data);
    delete[] init_data;
}

void Texture_As_Buffer::release()
{   
    if (p_texture_uav)
        p_texture_uav->Release();
    if (p_texture_srv)
        p_texture_srv->Release();
    if (p_texture)
        p_texture->Release();
    if (p_texture_staging)
        p_texture_staging->Release();
    if (data)
        delete[] ((unsigned char*)data);

    p_texture_srv = nullptr;
    p_texture_uav = nullptr;
    p_texture = nullptr;
    p_texture_staging = nullptr;
    data = nullptr;
}