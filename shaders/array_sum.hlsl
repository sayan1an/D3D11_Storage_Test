// ArraySum.hlsl
// Compute shader that demonstrates using the same buffer for both input and output

// Input buffer (read-only view)
StructuredBuffer<float> Input : register(t0);

// Output buffer (read-write view of the same buffer)
RWStructuredBuffer<float> Output : register(u0);

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, THREAD_GROUP_SIZE_Z)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // Get the current index
    uint idx = DTid.x;
    
    // Read from the input view and write to the output view
    // This demonstrates that we can read and write to the same buffer
    // through different views
    Output[idx] = Input[idx] + Input[idx];
} 