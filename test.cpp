#include "test.h"
#include "texture_as_buffer.h"
#include "d3d11_helper.h"
#include <iostream>
#include <DirectXPackedVector.h>
#include <cmath>
#include <string>

class Texture_As_Buffer_Write_Tester
{
public:
    void init(ID3D11Device* device, DXGI_FORMAT __test_fmt)
    {   
        m_device = device;
        m_width = 503;
        m_height = 250;
        size_t channels = 3;
        m_test_fmt = __test_fmt;

        m_tab.init(device, channels, m_height, m_width, m_test_fmt);
        m_tab.init_staging(device);

        const char* shader_code_r8_unorm = R"(
            RWTexture2DArray<unorm float> out_texture : register(u0);

            [numthreads(16, 16, 1)]
            void test_main(uint3 DTid : SV_DispatchThreadID)
            {
                int w_idx = DTid.x;
                int h_idx = DTid.y;

                int width;
                int height;
                int channels;

                out_texture.GetDimensions(width, height, channels);
            
                if (w_idx >= width || h_idx >= height)
                    return;
                
                for (int c = 0; c < channels; c++) {
                    uint index = (c * height * width + h_idx * width + w_idx) % 256;
                    int3 out_idx = int3(w_idx, h_idx, c);
                    out_texture[out_idx] = index / 254.9445f;
                }
            }
        )";

        const char* shader_code_r32_float = R"(
            RWTexture2DArray<float> out_texture : register(u0);

            [numthreads(16, 16, 1)]
            void test_main(uint3 DTid : SV_DispatchThreadID)
            {
                int w_idx = DTid.x;
                int h_idx = DTid.y;

                int width;
                int height;
                int channels;

                out_texture.GetDimensions(width, height, channels);
            
                if (w_idx >= width || h_idx >= height)
                    return;
                
                for (int c = 0; c < channels; c++) {
                    uint index = (c * height * width + h_idx * width + w_idx) % 256;
                    int3 out_idx = int3(w_idx, h_idx, c);
                    out_texture[out_idx] = index * (1.0f + 1.0f / 255.0f);
                }
            }
        )";

        const char* shader_code_r16_float = R"(
            RWTexture2DArray<half> out_texture : register(u0);

            [numthreads(16, 16, 1)]
            void test_main(uint3 DTid : SV_DispatchThreadID)
            {
                int w_idx = DTid.x;
                int h_idx = DTid.y;

                int width;
                int height;
                int channels;

                out_texture.GetDimensions(width, height, channels);
            
                if (w_idx >= width || h_idx >= height)
                    return;
                
                for (int c = 0; c < channels; c++) {
                    uint index = (c * height * width + h_idx * width + w_idx) % 256;
                    int3 out_idx = int3(w_idx, h_idx, c);
                    out_texture[out_idx] = (half)(index * (0.1f + 1.0f / 255.0f));
                }
            }
        )";

        const char* shader_code_r16g16_float = R"(
            RWTexture2DArray<half2> out_texture : register(u0);

            [numthreads(16, 16, 1)]
            void test_main(uint3 DTid : SV_DispatchThreadID)
            {
                int w_idx = DTid.x;
                int h_idx = DTid.y;

                int width;
                int height;
                int channels;

                out_texture.GetDimensions(width, height, channels);
            
                if (w_idx >= width || h_idx >= height)
                    return;
                
                for (int c = 0; c < channels; c++) {
                    uint index = (c * height * width + h_idx * width + w_idx) % 256;
                    int3 out_idx = int3(w_idx, h_idx, c);
                    out_texture[out_idx] = half2(index * 0.1f, index * 0.05f);
                }
            }
        )";

        const char* shader_code_r10g10b10a2_unorm = R"(
            RWTexture2DArray<unorm float4> out_texture : register(u0);

            [numthreads(16, 16, 1)]
            void test_main(uint3 DTid : SV_DispatchThreadID)
            {
                int w_idx = DTid.x;
                int h_idx = DTid.y;

                int width;
                int height;
                int channels;

                out_texture.GetDimensions(width, height, channels);
            
                if (w_idx >= width || h_idx >= height)
                    return;
                
                for (int c = 0; c < channels; c++) {
                    uint index = (c * height * width + h_idx * width + w_idx) % 256;
                    int3 out_idx = int3(w_idx, h_idx, c);
                    out_texture[out_idx] = float4(index / 255.0f, (index + 23) / 255.0f, (index + 53)/ 255.0f, 0);
                }
            }
        )";

        switch (m_test_fmt)
        {
            case DXGI_FORMAT_R32_FLOAT:
                m_compute_shader.init_from_code_string(device, shader_code_r32_float, "test_main");
                break;
            case DXGI_FORMAT_R8_UNORM:
                m_compute_shader.init_from_code_string(device, shader_code_r8_unorm, "test_main");
                break;
            case DXGI_FORMAT_R16_FLOAT:
                m_compute_shader.init_from_code_string(device, shader_code_r16_float, "test_main");
                break;
            case DXGI_FORMAT_R16G16_FLOAT:
                m_compute_shader.init_from_code_string(device, shader_code_r16g16_float, "test_main");
                break;
            case DXGI_FORMAT_R10G10B10A2_UNORM:
                m_compute_shader.init_from_code_string(device, shader_code_r10g10b10a2_unorm, "test_main");
                break;
            default:
                std::cout << "Undefined test format." << std::endl;
                break;
        }       
    }

    void execute(ID3D11DeviceContext* context)
    {
        context->CSSetShader(m_compute_shader.shader, nullptr, 0);
        // Bind UAV to register(u0)
        context->CSSetUnorderedAccessViews(0, 1, &m_tab.p_texture_uav, nullptr);
        // Dispatch
        UINT dispatchX = (m_width + 16 - 1) / 16;
        UINT dispatchY = (m_height + 16 - 1) / 16;
        context->Dispatch(dispatchX, dispatchY, 1);
        
        // Cleanup - unbind UAV
        ID3D11UnorderedAccessView* nullUAV[1] = { nullptr };
        context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
    }

    void test(ID3D11DeviceContext* context)
    {
        void *data = m_tab.to_cpu(context);
       
        switch (m_test_fmt)
        {
        case DXGI_FORMAT_R32_FLOAT:
            test_r32_float(data);
            break;
        case DXGI_FORMAT_R8_UNORM:
            test_r8_unorm(data);
            break;
        case DXGI_FORMAT_R16_FLOAT:
            test_r16_float(data);
            break;
        case DXGI_FORMAT_R16G16_FLOAT:
            test_r16g16_float(data);
            break;
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            test_r10g10b10a2_unorm(data);
            break;
        default:
            std::cout << "Undefined test format." << std::endl;
            break;
        }
    }

    void release()
    {
        m_tab.release();
        m_compute_shader.release();
    }

private:
    ID3D11Device* m_device;
    D3D11_Compute_Shader m_compute_shader;
    Texture_As_Buffer m_tab;
    unsigned int m_width = 0;
    unsigned int m_height = 0;
    DXGI_FORMAT m_test_fmt;

    void test_r8_unorm(const void* data)
    {   
        float error = 0;
        for (UINT c_idx = 0; c_idx < m_tab.channels; c_idx++)
            for (UINT h_idx = 0; h_idx < m_tab.height; h_idx++)
                for (UINT w_idx = 0; w_idx < m_tab.width; w_idx++) {
                    unsigned int input = (unsigned int)((uint8_t*)data)[m_tab.width * m_tab.height * c_idx + m_tab.width * h_idx + w_idx];
                    unsigned int expected = (c_idx * m_tab.height * m_tab.width + h_idx * m_tab.width + w_idx) % 256;
                    error += abs((int)input - (int)expected);
                }

        if (error == 0.0f)
            std::cout << "Test R8_UNORM passed!" << std::endl;
        else
            std::cout << "Test R8_UNORM failed! Error: " << error << std::endl;
    }

    void test_r32_float(const void* data)
    {
        float error = 0;
        for (UINT c_idx = 0; c_idx < m_tab.channels; c_idx++)
            for (UINT h_idx = 0; h_idx < m_tab.height; h_idx++)
                for (UINT w_idx = 0; w_idx < m_tab.width; w_idx++) {
                    float input = ((float *)data)[m_tab.width * m_tab.height * c_idx + m_tab.width * h_idx + w_idx];
                    float expected = ((c_idx * m_tab.height * m_tab.width + h_idx * m_tab.width + w_idx) % 256) * (1.0f + 1.0f / 255.0f);
                    error += abs(input - expected);
                }
        
        if (error < 1e-5f)
            std::cout << "Test R32_FLOAT passed!" << std::endl;
        else
            std::cout << "Test R32_FLOAT failed! Error: " << error << std::endl;
    }

    void test_r16_float(const void* data)
    {
        float error = 0;
        for (UINT c_idx = 0; c_idx < m_tab.channels; c_idx++)
            for (UINT h_idx = 0; h_idx < m_tab.height; h_idx++)
                for (UINT w_idx = 0; w_idx < m_tab.width; w_idx++) {
                    uint16_t input_rpr = ((uint16_t*)data)[m_tab.width * m_tab.height * c_idx + m_tab.width * h_idx + w_idx];
                    float input = DirectX::PackedVector::XMConvertHalfToFloat(input_rpr);
                    float expected = DirectX::PackedVector::XMConvertHalfToFloat(DirectX::PackedVector::XMConvertFloatToHalf(((c_idx * m_tab.height * m_tab.width + h_idx * m_tab.width + w_idx) % 256) * (0.1f + 1.0f / 255.0f)));
                    error += abs(input - expected);
                }
        
        error /= (m_tab.channels * m_tab.height * m_tab.width);

        if (error < 5e-3f)
            std::cout << "Test R16_FLOAT passed!" << std::endl;
        else
            std::cout << "Test R16_FLOAT failed! Error: " << error << std::endl;
    }

    void test_r16g16_float(const void* data)
    {
        float error = 0;
                
        struct H2
        {
            UINT16 r;
            UINT16 g;
        };
              
        for (UINT c_idx = 0; c_idx < m_tab.channels; c_idx++)
            for (UINT h_idx = 0; h_idx < m_tab.height; h_idx++)
                for (UINT w_idx = 0; w_idx < m_tab.width; w_idx++) {
                    H2 input_rpr = ((H2 *)data)[m_tab.width * m_tab.height * c_idx + m_tab.width * h_idx + w_idx];
                    float input_r = DirectX::PackedVector::XMConvertHalfToFloat(input_rpr.r);
                    float input_g = DirectX::PackedVector::XMConvertHalfToFloat(input_rpr.g);
                    float expected_f_r = ((c_idx * m_tab.height * m_tab.width + h_idx * m_tab.width + w_idx) % 256) * 0.1f;
                    float expected_f_g = ((c_idx * m_tab.height * m_tab.width + h_idx * m_tab.width + w_idx) % 256) * 0.05f;
                    float expected_r = DirectX::PackedVector::XMConvertHalfToFloat(DirectX::PackedVector::XMConvertFloatToHalf(expected_f_r));
                    float expected_g = DirectX::PackedVector::XMConvertHalfToFloat(DirectX::PackedVector::XMConvertFloatToHalf(expected_f_g));
                    error += abs(input_r - expected_r);
                    error += abs(input_g - expected_g);
                }

        error /= (m_tab.channels * m_tab.height * m_tab.width * 2);

        if (error < 5e-3f)
            std::cout << "Test R16G16_FLOAT passed!" << std::endl;
        else
            std::cout << "Test R16G16_FLOAT failed! Error: " << error << std::endl;
    }
    
    void test_r10g10b10a2_unorm(const void* data)
    {
        float error = 0;
        for (UINT c_idx = 0; c_idx < m_tab.channels; c_idx++)
            for (UINT h_idx = 0; h_idx < m_tab.height; h_idx++)
                for (UINT w_idx = 0; w_idx < m_tab.width; w_idx++) {
                    unsigned int input_repr = ((unsigned int*)data)[m_tab.width * m_tab.height * c_idx + m_tab.width * h_idx + w_idx];
                    unsigned int tmp = ((c_idx * m_tab.height * m_tab.width + h_idx * m_tab.width + w_idx) % 256);
                    float input_r = (input_repr & 0x3FF) / 1023.0f;
                    float input_g = ((input_repr >> 10) & 0x3FF) / 1023.0f;
                    float input_b = ((input_repr >> 20) & 0x3FF) / 1023.0f;
                    error += abs(input_r - max(0.0f, min(1.0f, tmp / 255.0f)));
                    error += abs(input_g - max(0.0f, min(1.0f, (tmp + 23) / 255.0f)));
                    error += abs(input_b - max(0.0f, min(1.0f, (tmp + 53) / 255.0f)));
                }

        error /= (m_tab.channels * m_tab.height * m_tab.width * 3);

        if (error < 5e-4f)
            std::cerr << "Test R10G10B10A2_UNORM passed!" << std::endl;
        else
            std::cerr << "Test R10G10B10A2_UNORM failed! Error: " << error << std::endl;
    }
};

void run_write_test(ID3D11Device* device, ID3D11DeviceContext* context)
{   
    std::cerr << "Running write test..." << std::endl;
    Texture_As_Buffer_Write_Tester tester;
    tester.init(device, DXGI_FORMAT_R8_UNORM);
    tester.execute(context);
    tester.test(context);
    tester.release();

    tester.init(device, DXGI_FORMAT_R32_FLOAT);
    tester.execute(context);
    tester.test(context);
    tester.release();

    tester.init(device, DXGI_FORMAT_R16_FLOAT); 
    tester.execute(context);    
    tester.test(context);
    tester.release();

    tester.init(device, DXGI_FORMAT_R16G16_FLOAT);
    tester.execute(context);    
    tester.test(context);
    tester.release();

    tester.init(device, DXGI_FORMAT_R10G10B10A2_UNORM);
    tester.execute(context);
    tester.test(context);
    tester.release();
}

class Texture_As_Buffer_Read_Tester
{
public:
    void init(ID3D11Device* device, DXGI_FORMAT __test_fmt)
    {   
        m_device = device;
        m_width = 503;
        m_height = 250;
        size_t channels = 3;
        m_test_fmt = __test_fmt;

        m_tab_in.init(device, channels, m_height, m_width, m_test_fmt);
        m_tab_in.init_staging(device);
        m_tab_out.init(device, channels, m_height, m_width, DXGI_FORMAT_R32_FLOAT);
        m_tab_out.init_staging(device);

        const char* shader_code_r8_unorm = R"(
        Texture2DArray<unorm float> in_texture : register(t0);
        RWTexture2DArray<float> out_texture : register(u0);

        [numthreads(16, 16, 1)]
        void test_main(uint3 DTid : SV_DispatchThreadID)
        {
            int w_idx = DTid.x;
            int h_idx = DTid.y;

            int width;
            int height;
            int channels;

            out_texture.GetDimensions(width, height, channels);
        
            if (w_idx >= width || h_idx >= height)
                return;
            
            int h_idx_in = (h_idx + w_idx ^ h_idx) % height;
            int w_idx_in = (w_idx + w_idx ^ h_idx) % width;
            int h_idx_in_n = (h_idx + w_idx ^ h_idx + 1) % height;
            int w_idx_in_n = (w_idx + w_idx ^ h_idx + 1) % width;

            for (int c = 0; c < channels; c++) {
                int3 out_idx = int3(w_idx, h_idx, c);
                float input_0 = in_texture[int3(w_idx_in, h_idx_in, c)];
                float input_1 = in_texture[int3(w_idx_in_n, h_idx_in, c)];
                float input_2 = in_texture[int3(w_idx_in, h_idx_in_n, c)];
                float input_3 = in_texture[int3(w_idx_in_n, h_idx_in_n, c)];
                out_texture[out_idx] = input_0 + input_1 + input_2 + input_3;
            }
        }
        )";

        const char* shader_code_r32_float = R"(
        Texture2DArray<float> in_texture : register(t0);
        RWTexture2DArray<float> out_texture : register(u0);

        [numthreads(16, 16, 1)]
        void test_main(uint3 DTid : SV_DispatchThreadID)
        {
            int w_idx = DTid.x;
            int h_idx = DTid.y;

            int width;
            int height;
            int channels;

            out_texture.GetDimensions(width, height, channels);
        
            if (w_idx >= width || h_idx >= height)
                return;
            
            int h_idx_in = (h_idx + w_idx ^ h_idx) % height;
            int w_idx_in = (w_idx + w_idx ^ h_idx) % width;
            int h_idx_in_n = (h_idx + w_idx ^ h_idx + 1) % height;
            int w_idx_in_n = (w_idx + w_idx ^ h_idx + 1) % width;

            for (int c = 0; c < channels; c++) {
                int3 out_idx = int3(w_idx, h_idx, c);
                float input_0 = in_texture[int3(w_idx_in, h_idx_in, c)];
                float input_1 = in_texture[int3(w_idx_in_n, h_idx_in, c)];
                float input_2 = in_texture[int3(w_idx_in, h_idx_in_n, c)];
                float input_3 = in_texture[int3(w_idx_in_n, h_idx_in_n, c)];
                out_texture[out_idx] = input_0 + input_1 + input_2 + input_3;
            }
        }
        )";

        const char* shader_code_r16_float = R"(
        Texture2DArray<half> in_texture : register(t0);
        RWTexture2DArray<float> out_texture : register(u0);

        [numthreads(16, 16, 1)]
        void test_main(uint3 DTid : SV_DispatchThreadID)
        {
            int w_idx = DTid.x;
            int h_idx = DTid.y;

            int width;
            int height;
            int channels;

            out_texture.GetDimensions(width, height, channels);
        
            if (w_idx >= width || h_idx >= height)
                return;
            
            int h_idx_in = (h_idx + w_idx ^ h_idx) % height;
            int w_idx_in = (w_idx + w_idx ^ h_idx) % width;
            int h_idx_in_n = (h_idx + w_idx ^ h_idx + 1) % height;
            int w_idx_in_n = (w_idx + w_idx ^ h_idx + 1) % width;

            for (int c = 0; c < channels; c++) {
                int3 out_idx = int3(w_idx, h_idx, c);
                float input_0 = in_texture[int3(w_idx_in, h_idx_in, c)];
                float input_1 = in_texture[int3(w_idx_in_n, h_idx_in, c)];
                float input_2 = in_texture[int3(w_idx_in, h_idx_in_n, c)];
                float input_3 = in_texture[int3(w_idx_in_n, h_idx_in_n, c)];
                out_texture[out_idx] = input_0 + input_1 + input_2 + input_3;
            }
        }
        )";

        const char* shader_code_r16g16_float = R"(
        Texture2DArray<half2> in_texture : register(t0);
        RWTexture2DArray<float> out_texture : register(u0);

        [numthreads(16, 16, 1)]
        void test_main(uint3 DTid : SV_DispatchThreadID)
        {
            int w_idx = DTid.x;
            int h_idx = DTid.y;

            int width;
            int height;
            int channels;

            out_texture.GetDimensions(width, height, channels);
        
            if (w_idx >= width || h_idx >= height)
                return;
            
            int h_idx_in = (h_idx + w_idx ^ h_idx) % height;
            int w_idx_in = (w_idx + w_idx ^ h_idx) % width;
            int h_idx_in_n = (h_idx + w_idx ^ h_idx + 1) % height;
            int w_idx_in_n = (w_idx + w_idx ^ h_idx + 1) % width;

            for (int c = 0; c < channels; c++) {
                int3 out_idx = int3(w_idx, h_idx, c);
                float2 input_0 = in_texture[int3(w_idx_in, h_idx_in, c)];
                float2 input_1 = in_texture[int3(w_idx_in_n, h_idx_in, c)];
                float2 input_2 = in_texture[int3(w_idx_in, h_idx_in_n, c)];
                float2 input_3 = in_texture[int3(w_idx_in_n, h_idx_in_n, c)];
                float2 output = input_0 + input_1 + input_2 + input_3;
                out_texture[out_idx] = output.r + output.g;
            }
        }
        )";

        const char* shader_code_r10g10b10a2_unorm = R"(
        Texture2DArray<unorm float4> in_texture : register(t0);
        RWTexture2DArray<float> out_texture : register(u0);

        [numthreads(16, 16, 1)]
        void test_main(uint3 DTid : SV_DispatchThreadID)
        {
            int w_idx = DTid.x;
            int h_idx = DTid.y;

            int width;
            int height;
            int channels;

            out_texture.GetDimensions(width, height, channels);
        
            if (w_idx >= width || h_idx >= height)
                return;
            
            int h_idx_in = (h_idx + w_idx ^ h_idx) % height;
            int w_idx_in = (w_idx + w_idx ^ h_idx) % width;
            int h_idx_in_n = (h_idx + w_idx ^ h_idx + 1) % height;
            int w_idx_in_n = (w_idx + w_idx ^ h_idx + 1) % width;

            for (int c = 0; c < channels; c++) {
                int3 out_idx = int3(w_idx, h_idx, c);
                float4 input_0 = in_texture[int3(w_idx_in, h_idx_in, c)];
                float4 input_1 = in_texture[int3(w_idx_in_n, h_idx_in, c)];
                float4 input_2 = in_texture[int3(w_idx_in, h_idx_in_n, c)];
                float4 input_3 = in_texture[int3(w_idx_in_n, h_idx_in_n, c)];
                float4 output = input_0 + input_1 + input_2 + input_3;
                out_texture[out_idx] = output.r + output.g + output.b + output.a;
            }
        }
        )";

        switch (m_test_fmt)
        {
            case DXGI_FORMAT_R32_FLOAT:
                m_compute_shader.init_from_code_string(device, shader_code_r32_float, "test_main");
                break;
            case DXGI_FORMAT_R8_UNORM:
                m_compute_shader.init_from_code_string(device, shader_code_r8_unorm, "test_main");
                break;
            case DXGI_FORMAT_R16_FLOAT:
                m_compute_shader.init_from_code_string(device, shader_code_r16_float, "test_main");
                break;
            case DXGI_FORMAT_R16G16_FLOAT:
                m_compute_shader.init_from_code_string(device, shader_code_r16g16_float, "test_main");
                break;
            case DXGI_FORMAT_R10G10B10A2_UNORM:
                m_compute_shader.init_from_code_string(device, shader_code_r10g10b10a2_unorm, "test_main");
                break;
            default:
                std::cout << "Undefined test format." << std::endl;
                break;
        }       

    }
   
    void test(ID3D11DeviceContext* context)
    {       
          switch (m_test_fmt)
        {
            case DXGI_FORMAT_R32_FLOAT:
                test_r32_float(context);
                break;
            case DXGI_FORMAT_R8_UNORM:
                test_r8_unorm(context);
                break;
            case DXGI_FORMAT_R16_FLOAT:
                test_r16_float(context);
                break;
            case DXGI_FORMAT_R16G16_FLOAT:
                test_r16g16_float(context);
                break;
            case DXGI_FORMAT_R10G10B10A2_UNORM:
                test_r10g10b10a2_unorm(context);
                break;
            default:
                std::cout << "Undefined test format." << std::endl;
                break;
        }       
    }

    void release()
    {
        m_tab_in.release();
        m_tab_out.release();
        m_compute_shader.release();
    }
private:
    ID3D11Device* m_device;
    D3D11_Compute_Shader m_compute_shader;
    Texture_As_Buffer m_tab_in;
    Texture_As_Buffer m_tab_out;
    unsigned int m_width = 0;
    unsigned int m_height = 0;
    DXGI_FORMAT m_test_fmt;

    void execute(ID3D11DeviceContext* context)
    {
        context->CSSetShader(m_compute_shader.shader, nullptr, 0);

        // Bind SRV to register(t0)
        context->CSSetShaderResources(0, 1, &m_tab_in.p_texture_srv);
         
        // Bind UAV to register(u0)
        context->CSSetUnorderedAccessViews(0, 1, &m_tab_out.p_texture_uav, nullptr);
        // Dispatch
        UINT dispatchX = (m_width + 16 - 1) / 16;
        UINT dispatchY = (m_height + 16 - 1) / 16;
        context->Dispatch(dispatchX, dispatchY, 1);
        
        // Cleanup - unbind UAV
        ID3D11UnorderedAccessView* nullUAV[1] = { nullptr };
        context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
    }
 
    void test_r32_float(ID3D11DeviceContext* context)
    {   
        unsigned char *ref_data = new unsigned char[m_tab_in.width * m_tab_in.height * m_tab_in.channels * m_tab_in.element_size];
        for (UINT c_idx = 0; c_idx < m_tab_in.channels; c_idx++)
            for (UINT h_idx = 0; h_idx < m_tab_in.height; h_idx++)
                for (UINT w_idx = 0; w_idx < m_tab_in.width; w_idx++) {
                    ((float *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx + w_idx] = 1.2f + (c_idx ^ h_idx ^ w_idx);
                }

        m_tab_in.to_gpu(context, ref_data);
        execute(context);
        void *data = m_tab_out.to_cpu(context);
        float error = 0;
        for (UINT c_idx = 0; c_idx < m_tab_in.channels; c_idx++)
            for (UINT h_idx = 0; h_idx < m_tab_in.height; h_idx++)
                for (UINT w_idx = 0; w_idx < m_tab_in.width; w_idx++) {
                    UINT h_idx_in = (h_idx + w_idx ^ h_idx) % m_tab_in.height;
                    UINT w_idx_in = (w_idx + w_idx ^ h_idx) % m_tab_in.width;
                    UINT h_idx_in_n = (h_idx + w_idx ^ h_idx + 1) % m_tab_in.height;
                    UINT w_idx_in_n = (w_idx + w_idx ^ h_idx + 1) % m_tab_in.width;
                    float expected_0 = ((float *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in + w_idx_in];
                    float expected_1 = ((float *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in + w_idx_in_n];
                    float expected_2 = ((float *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in_n + w_idx_in];
                    float expected_3 = ((float *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in_n + w_idx_in_n];
                    float input = ((float *)data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx + w_idx];
                    error += abs(input - (expected_0 + expected_1 + expected_2 + expected_3));
                }

        error /= (m_tab_in.channels * m_tab_in.height * m_tab_in.width);

        if (error < 1e-5f)
            std::cout << "Test R32_FLOAT passed!" << std::endl;
        else
            std::cout << "Test R32_FLOAT failed! Error: " << error << std::endl;

         delete[] ((unsigned char*)ref_data);
    }

    void test_r8_unorm(ID3D11DeviceContext* context)
    {   
        unsigned char *ref_data = new unsigned char[m_tab_in.width * m_tab_in.height * m_tab_in.channels * m_tab_in.element_size];
        for (UINT c_idx = 0; c_idx < m_tab_in.channels; c_idx++)
            for (UINT h_idx = 0; h_idx < m_tab_in.height; h_idx++)
                for (UINT w_idx = 0; w_idx < m_tab_in.width; w_idx++)
                    ref_data[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx + w_idx] = (c_idx ^ h_idx ^ w_idx) % 255;
                

        m_tab_in.to_gpu(context, ref_data);
        execute(context);
        void *data = m_tab_out.to_cpu(context);
        float error = 0;
        for (UINT c_idx = 0; c_idx < m_tab_in.channels; c_idx++)
            for (UINT h_idx = 0; h_idx < m_tab_in.height; h_idx++)
                for (UINT w_idx = 0; w_idx < m_tab_in.width; w_idx++) {
                    UINT h_idx_in = (h_idx + w_idx ^ h_idx) % m_tab_in.height;
                    UINT w_idx_in = (w_idx + w_idx ^ h_idx) % m_tab_in.width;
                    UINT h_idx_in_n = (h_idx + w_idx ^ h_idx + 1) % m_tab_in.height;
                    UINT w_idx_in_n = (w_idx + w_idx ^ h_idx + 1) % m_tab_in.width;
                    float expected_0 = ref_data[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in + w_idx_in] / 255.0f;
                    float expected_1 = ref_data[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in + w_idx_in_n] / 255.0f;
                    float expected_2 = ref_data[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in_n + w_idx_in] / 255.0f;
                    float expected_3 = ref_data[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in_n + w_idx_in_n] / 255.0f;
                    float input = ((float *)data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx + w_idx];
                    error += abs(input - (expected_0 + expected_1 + expected_2 + expected_3));
                }

        error /= (m_tab_in.channels * m_tab_in.height * m_tab_in.width);

        if (error < 1e-5f)
            std::cout << "Test R8_UNORM passed!" << std::endl;
        else
            std::cout << "Test R8_UNORM failed! Error: " << error << std::endl;

         delete[] ((unsigned char*)ref_data);
    }

    void test_r16_float(ID3D11DeviceContext* context)
    {   
        unsigned char *ref_data = new unsigned char[m_tab_in.width * m_tab_in.height * m_tab_in.channels * m_tab_in.element_size];
        for (UINT c_idx = 0; c_idx < m_tab_in.channels; c_idx++)
            for (UINT h_idx = 0; h_idx < m_tab_in.height; h_idx++)
                for (UINT w_idx = 0; w_idx < m_tab_in.width; w_idx++) {
                    ((uint16_t *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx + w_idx] = DirectX::PackedVector::XMConvertFloatToHalf(1.2f + (c_idx ^ h_idx ^ w_idx));
                }

        m_tab_in.to_gpu(context, ref_data);
        execute(context);
        void *data = m_tab_out.to_cpu(context);
        float error = 0;
        for (UINT c_idx = 0; c_idx < m_tab_in.channels; c_idx++)
            for (UINT h_idx = 0; h_idx < m_tab_in.height; h_idx++)
                for (UINT w_idx = 0; w_idx < m_tab_in.width; w_idx++) {
                    UINT h_idx_in = (h_idx + w_idx ^ h_idx) % m_tab_in.height;
                    UINT w_idx_in = (w_idx + w_idx ^ h_idx) % m_tab_in.width;
                    UINT h_idx_in_n = (h_idx + w_idx ^ h_idx + 1) % m_tab_in.height;
                    UINT w_idx_in_n = (w_idx + w_idx ^ h_idx + 1) % m_tab_in.width;
                    float expected_0 = DirectX::PackedVector::XMConvertHalfToFloat(((uint16_t *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in + w_idx_in]);
                    float expected_1 = DirectX::PackedVector::XMConvertHalfToFloat(((uint16_t *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in + w_idx_in_n]);
                    float expected_2 = DirectX::PackedVector::XMConvertHalfToFloat(((uint16_t *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in_n + w_idx_in]);
                    float expected_3 = DirectX::PackedVector::XMConvertHalfToFloat(((uint16_t *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in_n + w_idx_in_n]);
                    float input = ((float *)data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx + w_idx];
                    error += abs(input - (expected_0 + expected_1 + expected_2 + expected_3));
                }

        error /= (m_tab_in.channels * m_tab_in.height * m_tab_in.width);

        if (error < 1e-5f)
            std::cout << "Test R16_FLOAT passed!" << std::endl;
        else
            std::cout << "Test R16_FLOAT failed! Error: " << error << std::endl;

         delete[] ((unsigned char*)ref_data);
    }

    struct F2
    {
        float r;
        float g;
    };

    struct H2
    {
        UINT16 r;
        UINT16 g;
    };

    F2 half2_to_float2(H2 h2)
    {
        return F2{DirectX::PackedVector::XMConvertHalfToFloat(h2.r), DirectX::PackedVector::XMConvertHalfToFloat(h2.g)};
    }

    void test_r16g16_float(ID3D11DeviceContext* context)
    {       
        unsigned char *ref_data = new unsigned char[m_tab_in.width * m_tab_in.height * m_tab_in.channels * m_tab_in.element_size];
        for (UINT c_idx = 0; c_idx < m_tab_in.channels; c_idx++)
            for (UINT h_idx = 0; h_idx < m_tab_in.height; h_idx++)
                for (UINT w_idx = 0; w_idx < m_tab_in.width; w_idx++) {
                    H2 h2;
                    h2.r = DirectX::PackedVector::XMConvertFloatToHalf(1.2f + (c_idx ^ h_idx ^ w_idx));
                    h2.g = DirectX::PackedVector::XMConvertFloatToHalf(2.8f + (c_idx ^ h_idx ^ w_idx));
                    ((H2 *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx + w_idx] = h2;
                }

        m_tab_in.to_gpu(context, ref_data);
        execute(context);
        void *data = m_tab_out.to_cpu(context);
        float error = 0;
        for (UINT c_idx = 0; c_idx < m_tab_in.channels; c_idx++)
            for (UINT h_idx = 0; h_idx < m_tab_in.height; h_idx++)
                for (UINT w_idx = 0; w_idx < m_tab_in.width; w_idx++) {
                    UINT h_idx_in = (h_idx + w_idx ^ h_idx) % m_tab_in.height;
                    UINT w_idx_in = (w_idx + w_idx ^ h_idx) % m_tab_in.width;
                    UINT h_idx_in_n = (h_idx + w_idx ^ h_idx + 1) % m_tab_in.height;
                    UINT w_idx_in_n = (w_idx + w_idx ^ h_idx + 1) % m_tab_in.width;
                    F2 expected_0 = half2_to_float2(((H2 *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in + w_idx_in]);
                    F2 expected_1 = half2_to_float2(((H2 *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in + w_idx_in_n]);
                    F2 expected_2 = half2_to_float2(((H2 *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in_n + w_idx_in]);
                    F2 expected_3 = half2_to_float2(((H2 *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in_n + w_idx_in_n]);
                    float input = ((float *)data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx + w_idx];
                    error += abs(input - (expected_0.r + expected_1.r + expected_2.r + expected_3.r + expected_0.g + expected_1.g + expected_2.g + expected_3.g));
                }

        error /= (m_tab_in.channels * m_tab_in.height * m_tab_in.width);

        if (error < 1e-5f)
            std::cout << "Test R16G16_FLOAT passed!" << std::endl;
        else
            std::cout << "Test R16G16_FLOAT failed! Error: " << error << std::endl;

         delete[] ((unsigned char*)ref_data);
    }

    struct F4
    {
        float r;
        float g;
        float b;
        float a;
    };

    F4 rgb10a2_to_float4(UINT u)
    {
        return F4{
            (float)(u >> 0 & 0x3ff) / 1023.0f,
            (float)(u >> 10 & 0x3ff) / 1023.0f,
            (float)(u >> 20 & 0x3ff) / 1023.0f,
            (float)(u >> 30) / 3.0f};
    }
    
    void test_r10g10b10a2_unorm(ID3D11DeviceContext* context)
    {   
        unsigned char *ref_data = new unsigned char[m_tab_in.width * m_tab_in.height * m_tab_in.channels * m_tab_in.element_size];
        for (UINT c_idx = 0; c_idx < m_tab_in.channels; c_idx++)
            for (UINT h_idx = 0; h_idx < m_tab_in.height; h_idx++)
                for (UINT w_idx = 0; w_idx < m_tab_in.width; w_idx++)
                    ((UINT *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx + w_idx] = ((c_idx << 30) ^ (h_idx << 20) ^ (w_idx << 10) ^ (w_idx + h_idx)) | 1;
                
        m_tab_in.to_gpu(context, ref_data);
        execute(context);
        void *data = m_tab_out.to_cpu(context);
        float error = 0;
        for (UINT c_idx = 0; c_idx < m_tab_in.channels; c_idx++)
            for (UINT h_idx = 0; h_idx < m_tab_in.height; h_idx++)
                for (UINT w_idx = 0; w_idx < m_tab_in.width; w_idx++) {
                    UINT h_idx_in = (h_idx + w_idx ^ h_idx) % m_tab_in.height;
                    UINT w_idx_in = (w_idx + w_idx ^ h_idx) % m_tab_in.width;
                    UINT h_idx_in_n = (h_idx + w_idx ^ h_idx + 1) % m_tab_in.height;
                    UINT w_idx_in_n = (w_idx + w_idx ^ h_idx + 1) % m_tab_in.width;
                    F4 expected_0 = rgb10a2_to_float4(((UINT *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in + w_idx_in]);
                    F4 expected_1 = rgb10a2_to_float4(((UINT *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in + w_idx_in_n]);
                    F4 expected_2 = rgb10a2_to_float4(((UINT *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in_n + w_idx_in]);
                    F4 expected_3 = rgb10a2_to_float4(((UINT *)ref_data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx_in_n + w_idx_in_n]);
                    float input = ((float *)data)[m_tab_in.width * m_tab_in.height * c_idx + m_tab_in.width * h_idx + w_idx];
                    error += abs(input - (expected_0.r + expected_1.r + expected_2.r + expected_3.r + expected_0.g + expected_1.g + expected_2.g + expected_3.g + expected_0.b + expected_1.b + expected_2.b + expected_3.b + expected_0.a + expected_1.a + expected_2.a + expected_3.a));
                }

        error /= (m_tab_in.channels * m_tab_in.height * m_tab_in.width);

        if (error < 1e-5f)
            std::cout << "Test R10G10B10A2_UNORM passed!" << std::endl;
        else
            std::cout << "Test R10G10B10A2_UNORM failed! Error: " << error << std::endl;

         delete[] ((unsigned char*)ref_data);
    }
};

void run_read_test(ID3D11Device* device, ID3D11DeviceContext* context)
{
    std::cerr << "Running read test..." << std::endl;
    Texture_As_Buffer_Read_Tester tester;
    tester.init(device, DXGI_FORMAT_R32_FLOAT);
    tester.test(context);
    tester.release();

    tester.init(device, DXGI_FORMAT_R8_UNORM);
    tester.test(context);
    tester.release();

    tester.init(device, DXGI_FORMAT_R16_FLOAT);
    tester.test(context);
    tester.release();

    tester.init(device, DXGI_FORMAT_R16G16_FLOAT);
    tester.test(context);
    tester.release();

    tester.init(device, DXGI_FORMAT_R10G10B10A2_UNORM);
    tester.test(context);
    tester.release();
}

class Shader_Compile_Tester
{
public:
    void init(ID3D11Device* device) {
        D3D_SHADER_MACRO defines[4] = {{ "THREAD_GROUP_SIZE_X", std::to_string(tid_x).c_str() }, { "THREAD_GROUP_SIZE_Y", std::to_string(tid_y).c_str() }, { "THREAD_GROUP_SIZE_Z", std::to_string(tid_z).c_str() }, { nullptr, nullptr }};
        m_compute_shader.init_from_file(device, "shaders/array_sum.hlsl", "main", defines);
    }
    //void test(ID3D11DeviceContext* context);
    void release() {
        m_compute_shader.release();
    }

private:
    D3D11_Compute_Shader m_compute_shader;
    const UINT tid_x = 16;
    const UINT tid_y = 1;
    const UINT tid_z = 1;
};

void run_shader_compile_test(ID3D11Device* device, ID3D11DeviceContext* context)
{
    std::cerr << "Running shader compile test..." << std::endl;
    Shader_Compile_Tester tester;
    tester.init(device);
    //tester.test(context);
    tester.release();
}