#define _WIN32_WINNT 0x600
#include <stdio.h>
#include <fstream>
#include <atlstr.h>
#include <iostream>
#include <d3dcompiler.h>

#pragma comment(lib,"d3dcompiler.lib")

HRESULT CompileShader(_In_ LPCWSTR src_file, _In_ LPCSTR entry_point, _In_ LPCSTR profile, _Outptr_ ID3DBlob** blob);

int CompileShaders(ID3DBlob** vs_blob, LPCWSTR vertex_file_path, ID3DBlob** ps_blob, LPCWSTR pixel_file_pth);
