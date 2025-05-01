#pragma once
#include <d3d11.h>

void run_write_test(ID3D11Device* device, ID3D11DeviceContext* context);
void run_read_test(ID3D11Device* device, ID3D11DeviceContext* context);
void run_shader_compile_test(ID3D11Device* device, ID3D11DeviceContext* context);

