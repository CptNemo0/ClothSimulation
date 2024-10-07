#include "../include/global.h"

#include <d3d11.h>
#include <tchar.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <wrl/client.h>
#include <xmmintrin.h>
#include <WinBase.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#include "../include/Shader.h"
#include "../include/ConstantBufferStructs.h"
// Data

#ifndef GET_X_LPARAM
    #define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
    #define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    swap_chain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    device->CreateRenderTargetView(pBackBuffer, NULL, &main_render_target_view);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (main_render_target_view) { main_render_target_view->Release(); main_render_target_view = NULL; }
}

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &swap_chain, &device, &featureLevel, &context) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (swap_chain) { swap_chain->Release(); swap_chain = NULL; }
    if (context) { context->Release(); context = NULL; }
    if (device) { device->Release(); device = NULL; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return true;
    }
        
    switch (msg)
    {
    case WM_SIZE:
        if (device != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            swap_chain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }

    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}


// Main code
int main(int, char**)
{
#pragma region Initialization

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("Cloth Simulation"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("ClothSimulation"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    ImGui_ImplWin32_Init(hwnd);  // Pass the window handle here
    ImGui_ImplDX11_Init(device, context);

    //===============================

    D3D11_VIEWPORT viewport = { 0 };
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = 1280;
    viewport.Height = 800;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    D3D11_RASTERIZER_DESC rasterizer_description;
    ZeroMemory(&rasterizer_description, sizeof(rasterizer_description));
    rasterizer_description.FillMode = D3D11_FILL_SOLID;
    rasterizer_description.CullMode = D3D11_CULL_NONE;
    rasterizer_description.FrontCounterClockwise = false;
    rasterizer_description.DepthClipEnable = true;

    ID3D11RasterizerState* rasterizer_state = nullptr;
    device->CreateRasterizerState(&rasterizer_description, &rasterizer_state);
    context->RSSetState(rasterizer_state);

    DirectX::XMVECTOR  camera_position_iv = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
    float camera_distance = 3.0f;
    DirectX::XMVECTOR  camera_angles = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR  center_position_iv = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    DirectX::XMVECTOR  up_direction_iv =    DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    auto view_matrix = DirectX::XMMatrixLookAtLH(camera_position_iv, center_position_iv, up_direction_iv);
    auto projection_matrix = DirectX::XMMatrixPerspectiveFovLH(0.7864f, 16.0f/9.0f, 0.1f, 1000.0f);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

#pragma endregion     
    
#pragma region Resources Initialization

    const glm::vec3 triangle_verticies[]
    {
        { 0.0f, 1.0f, 0.0f },
        { 1.0f, -1.0f, 0.0f },
        { -1.0f, -1.0f, 0.0f }
    };

    const unsigned int triangle_indices[]
    {
        0, 1, 2,
    };

    ComPtr<ID3D11Buffer> vertex_buffer;
    D3D11_BUFFER_DESC buffer_description = { 0 };
    buffer_description.ByteWidth = sizeof(float) * 9;
    buffer_description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vertex_srd = { triangle_verticies, 0, 0 };
    device->CreateBuffer(&buffer_description, &vertex_srd, &vertex_buffer);

    ComPtr<ID3D11Buffer> index_buffer;
    D3D11_BUFFER_DESC index_buffer_description = { 0 };
    index_buffer_description.ByteWidth = sizeof(unsigned int) * 3;
    index_buffer_description.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA index_srd = { triangle_indices, 0, 0 };
    device->CreateBuffer(&index_buffer_description, &index_srd, &index_buffer);

    ComPtr<ID3D11VertexShader> vertex_shader;
    ComPtr<ID3D11PixelShader> pixel_shader;
    ID3DBlob* vs_blob = nullptr;
    ID3DBlob* ps_blob = nullptr;
    CompileShaders(&vs_blob, L"../res/Shaders/VertexShader.hlsl", &ps_blob, L"../res/Shaders/PixelShader.hlsl");
    device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &vertex_shader);
    device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &pixel_shader);

    context->VSSetShader(vertex_shader.Get(), nullptr, 0);
    context->PSSetShader(pixel_shader.Get(), nullptr, 0);

    ColorBuffer color_data;
    color_data.color = DirectX::XMFLOAT4(1.0, 0.0, 1.0, 1.0);

    ViewProjBuffer view_proj_data;
    view_proj_data.view_matrix = view_matrix;
    view_proj_data.projection_matrix = projection_matrix;

    ID3D11Buffer* color_constant_buffer;
    D3D11_BUFFER_DESC color_constant_buffer_description = { 0 };
    color_constant_buffer_description.ByteWidth = sizeof(ColorBuffer);
    color_constant_buffer_description.Usage = D3D11_USAGE_DYNAMIC;
    color_constant_buffer_description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    color_constant_buffer_description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    D3D11_SUBRESOURCE_DATA color_constant_buffer_srd;
    color_constant_buffer_srd.pSysMem = &color_data;
    color_constant_buffer_srd.SysMemPitch = 0;
    color_constant_buffer_srd.SysMemSlicePitch = 0;
    device->CreateBuffer(&color_constant_buffer_description, &color_constant_buffer_srd, &color_constant_buffer);
    context->PSSetConstantBuffers(0, 1, &color_constant_buffer);

    ID3D11Buffer* view_proj_constant_buffer;
    D3D11_BUFFER_DESC view_proj_constant_buffer_description = { 0 };
    view_proj_constant_buffer_description.ByteWidth = sizeof(ViewProjBuffer);
    view_proj_constant_buffer_description.Usage = D3D11_USAGE_DYNAMIC;
    view_proj_constant_buffer_description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    view_proj_constant_buffer_description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    D3D11_SUBRESOURCE_DATA view_proj_constant_buffer_srd;
    view_proj_constant_buffer_srd.pSysMem = &view_proj_data;
    view_proj_constant_buffer_srd.SysMemPitch = 0;
    view_proj_constant_buffer_srd.SysMemSlicePitch = 0;
    device->CreateBuffer(&view_proj_constant_buffer_description, &view_proj_constant_buffer_srd, &view_proj_constant_buffer);
    context->VSSetConstantBuffers(1, 1, &view_proj_constant_buffer);

    // initialize input layout
    D3D11_INPUT_ELEMENT_DESC input_element_description[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    ComPtr<ID3D11InputLayout> input_layout = nullptr;
    device->CreateInputLayout(input_element_description, 1, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &input_layout);
    context->IASetInputLayout(input_layout.Get());

    UINT stride = sizeof(float) * 3;
    UINT offset = 0;

#pragma endregion
 
    // Main loop
    bool done = false;
    bool rotate = false;
    POINT prev_mouse_pos = { 0, 0 };
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
            if (msg.message == WM_MBUTTONDOWN)
            {
                rotate = true;
            }
            if (msg.message == WM_MBUTTONUP)
            {
                rotate = false;
            }
            if (msg.message == WM_MOUSEWHEEL)
            {
                float scroll_amount = GET_WHEEL_DELTA_WPARAM(msg.wParam) * 0.002f;
                camera_distance -= scroll_amount;

                float x = DirectX::XMScalarCos(DirectX::XMVectorGetByIndex(camera_angles, 0)) * DirectX::XMScalarCos(DirectX::XMVectorGetByIndex(camera_angles, 1));
                float y = DirectX::XMScalarSin(DirectX::XMVectorGetByIndex(camera_angles, 1));
                float z = -1.0 * DirectX::XMScalarSin(DirectX::XMVectorGetByIndex(camera_angles, 0)) * DirectX::XMScalarCos(DirectX::XMVectorGetByIndex(camera_angles, 1));
                camera_position_iv = DirectX::XMVectorSet(camera_distance * x, camera_distance * y, camera_distance * z, 1.0f);
                view_proj_data.view_matrix = DirectX::XMMatrixLookAtLH(camera_position_iv, center_position_iv, up_direction_iv);
            }
            if (msg.message = WM_MOUSEMOVE)
            {
                int mouseX = GET_X_LPARAM(msg.lParam);
                int mouseY = GET_Y_LPARAM(msg.lParam);

                float deltaX = (mouseX - prev_mouse_pos.x) * 0.01f;
                float deltaY = (mouseY - prev_mouse_pos.y) * 0.01f;

                prev_mouse_pos.x = mouseX;
                prev_mouse_pos.y = mouseY;

                if (rotate)
                {
                    float a = DirectX::XMVectorGetByIndex(camera_angles, 0) + deltaX;
                    float b = DirectX::XMVectorGetByIndex(camera_angles, 1) + deltaY;

                    camera_angles = DirectX::XMVectorSetByIndex(camera_angles, a, 0);
                    camera_angles = DirectX::XMVectorSetByIndex(camera_angles, b, 1);

                    float x = DirectX::XMScalarCos(a) * DirectX::XMScalarCos(b);
                    float y = DirectX::XMScalarSin(b);
                    float z = -1.0 * DirectX::XMScalarSin(a) * DirectX::XMScalarCos(b);
                    camera_position_iv = DirectX::XMVectorSet(camera_distance * x, camera_distance * y, camera_distance * z, 1.0f);
                    view_proj_data.view_matrix = DirectX::XMMatrixLookAtLH(camera_position_iv, center_position_iv, up_direction_iv);
                }
            }
        }
        if (done)
        {
            break;
        }

        const float clear_color_with_alpha[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
        context->OMSetRenderTargets(1, &main_render_target_view, NULL);
        context->ClearRenderTargetView(main_render_target_view, clear_color_with_alpha);

        //======================== Logic

        D3D11_MAPPED_SUBRESOURCE mapped_resource;
        if (color_constant_buffer)
        {
            context->Map(color_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
            ColorBuffer* data_ptr1 = (ColorBuffer*)mapped_resource.pData;
            *data_ptr1 = color_data;  // Update the data
            context->Unmap(color_constant_buffer, 0);
        }
        
        if (view_proj_constant_buffer)
        {
            context->Map(view_proj_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
            ViewProjBuffer* data_ptr2 = (ViewProjBuffer*)mapped_resource.pData;
            *data_ptr2 = view_proj_data;
            context->Unmap(view_proj_constant_buffer, 0);
        }
        
        context->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &stride, &offset);
        context->IASetIndexBuffer(index_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        context->VSSetConstantBuffers(1, 1, &view_proj_constant_buffer);
        context->DrawIndexed(3, 0, 0);
         
        //======================== End Logic


#pragma region IMGUI

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Color");
        float color_picker[4] = { color_data.color.x, color_data.color.y, color_data.color.z, color_data.color.w };
        if (ImGui::ColorPicker4("Pick a color", color_picker))
        {
            color_data.color = DirectX::XMFLOAT4(color_picker[0], color_picker[1], color_picker[2], color_picker[3]);
        }
        ImGui::End();

        ImGui::Render();
        context->OMSetRenderTargets(1, &main_render_target_view, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

#pragma endregion

        swap_chain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}