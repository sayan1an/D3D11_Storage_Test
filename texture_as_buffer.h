#pragma once
#include <d3d11.h>
#include <string>

/* 
 * Interface for TextureArray and RWTextureArray 
 */
struct Texture_As_Buffer
{   
    size_t channels = 0;
    size_t height = 0;
    size_t width = 0;
    size_t element_size = 0;
    ID3D11Texture2D* p_texture = nullptr;
    // Default views (same format as texture)
    ID3D11UnorderedAccessView *p_texture_uav = nullptr;
    ID3D11ShaderResourceView *p_texture_srv = nullptr;
        
    // Init texture and default views
    void init(ID3D11Device* device, size_t __channels, size_t __height, size_t __width, DXGI_FORMAT format = DXGI_FORMAT_R8_UNORM);
    // Init staging textures for host->device and device->host transfer
    void init_staging(ID3D11Device* device);
    // Fetch data from device and return host pointer
    void* to_cpu(ID3D11DeviceContext* context);
    // Clear device memory per 8-bit (same as memset)
    void to_gpu(ID3D11DeviceContext* context, unsigned char clear_val);
    // Clear device memory per 32-bit
    void to_gpu(ID3D11DeviceContext* context, unsigned int clear_val);
    // Update device memory with raw byte stream
    void to_gpu(ID3D11DeviceContext* context, void *data);
    // Release all memory
    void release();

    std::string print_shape()
    {
       return std::to_string(channels) + " " + std::to_string(height) + " " + std::to_string(width);
    }

    // Destructor
    ~Texture_As_Buffer()
    {
        release();
    }
private:
    ID3D11Texture2D* p_texture_staging = nullptr;
    void* data = nullptr;
};