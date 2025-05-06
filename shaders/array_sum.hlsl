cbuffer Constant_Buffer : register(b0)
{
    unsigned int time_index;
    int height;
    int width;
    int align_padding;
};

Texture2DArray<float> input_0 : register(t0);
Texture2DArray<float> input_1 : register(t1);

RWTexture2DArray<float> output : register(u0);

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    int w_idx = DTid.x;
    int h_idx = DTid.y;
    
    int width, height, channels;
    input_0.GetDimensions(width, height, channels);

    if (w_idx >= width || h_idx >= height)
        return;

    for (int c_idx = 0; c_idx < channels; c_idx++) {
        int3 idx = int3(w_idx, h_idx, c_idx);
        output[idx] = input_0[idx] + input_1[idx] + THREAD_GROUP_SIZE_X + THREAD_GROUP_SIZE_Y + time_index + height + width;
    }
} 