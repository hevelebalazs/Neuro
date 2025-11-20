#define UNICODE
#include <windows.h>

#include <d3d11_1.h>
#pragma comment(lib, "d3d11.lib")

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

#define Assert(condition) { if(!(condition)) { DebugBreak(); } }

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
	switch(message) {
		case WM_KEYDOWN: {
			if(wparam == VK_ESCAPE) {
				DestroyWindow(window);
			}
			break;
		}
		case WM_DESTROY: {
			PostQuitMessage(0);
			break;
		}
		default: {
			return DefWindowProcW(window, message, wparam, lparam);
		}
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int) {
	WNDCLASSEXW win_class = {};
	win_class.cbSize = sizeof(WNDCLASSEXW);
	win_class.style = CS_HREDRAW | CS_VREDRAW;
	win_class.lpfnWndProc = &WindowProc;
	win_class.hInstance = instance;
	win_class.hIcon = LoadIconW(0, IDI_APPLICATION);
	win_class.hCursor = LoadCursorW(0, IDC_ARROW);
	win_class.lpszClassName = L"NeuroWindowClass";
	win_class.hIconSm = LoadIconW(0, IDI_APPLICATION);

	if(!RegisterClassExW(&win_class)) {
		Assert(false);
		return GetLastError();
	}

	RECT rect = {0, 0, 1024, 768};
	AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);
	LONG width = rect.right - rect.left;
	LONG height = rect.bottom - rect.top;

	HWND window = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW,
								  win_class.lpszClassName,
								  L"Neuro",
								  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
								  CW_USEDEFAULT, CW_USEDEFAULT,
								  width, height, 0, 0, instance, 0);

	if(!window) {
		Assert(false);
		return GetLastError();
	}

	ID3D11Device *device = 0;
	ID3D11DeviceContext *device_context = 0;
	{
		ID3D11Device *device0 = 0;
		ID3D11DeviceContext *device_context0 = 0;
		D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };
		UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

		HRESULT result = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE,
										   0, creation_flags,
										   feature_levels, ARRAYSIZE(feature_levels),
										   D3D11_SDK_VERSION, &device0,
										   0, &device_context0);

		if(FAILED(result)) {
			Assert(false);
			return GetLastError();
		}

		result = device0->QueryInterface(__uuidof(ID3D11Device1), (void **)&device);
		Assert(SUCCEEDED(result));
		device0->Release();

		result = device_context0->QueryInterface(__uuidof(ID3D11DeviceContext1), (void **)&device_context);
		Assert(SUCCEEDED(result));
		device_context0->Release();
	}

	IDXGISwapChain1 *swap_chain = 0;
	{
		IDXGIFactory2 *dxgi_factory = 0;
		{
			IDXGIDevice1 *dxgi_device = 0;
			HRESULT result = device->QueryInterface(__uuidof(IDXGIDevice1), (void **)&dxgi_device);
			Assert(SUCCEEDED(result));

			IDXGIAdapter *dxgi_adapter = 0;
			result = dxgi_device->GetAdapter(&dxgi_adapter);
			Assert(SUCCEEDED(result));
			dxgi_device->Release();

			DXGI_ADAPTER_DESC adapter_desc = {};
			dxgi_adapter->GetDesc(&adapter_desc);

			result = dxgi_adapter->GetParent(__uuidof(IDXGIFactory2), (void **)&dxgi_factory);
			Assert(SUCCEEDED(result));
			dxgi_adapter->Release();
		}

		DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
		swap_chain_desc.Width = 0;
		swap_chain_desc.Height = 0;
		swap_chain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		swap_chain_desc.SampleDesc.Count = 1;
		swap_chain_desc.SampleDesc.Quality = 0;
		swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swap_chain_desc.BufferCount = 2;
		swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
		swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swap_chain_desc.Flags = 0;

		HRESULT result = dxgi_factory->CreateSwapChainForHwnd(device, window, &swap_chain_desc, 0, 0, &swap_chain);
		Assert(SUCCEEDED(result));

		dxgi_factory->Release();
	}

	ID3D11RenderTargetView *frame_buffer_view = 0;
	{
		ID3D11Texture2D *frame_buffer = 0;
		HRESULT result = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&frame_buffer);
		Assert(SUCCEEDED(result));

		result = device->CreateRenderTargetView(frame_buffer, 0, &frame_buffer_view);
		Assert(SUCCEEDED(result));
		frame_buffer->Release();
	}

	ID3DBlob *vs_blob = 0;
	ID3D11VertexShader *vertex_shader = 0;
	{
		ID3DBlob *error_blob = 0;
		HRESULT result = D3DCompileFromFile(L"../shaders.hlsl", 0, 0, "vs_main", "vs_5_0", 0, 0, &vs_blob, &error_blob);
		if(FAILED(result)) {
			if(result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
				Assert(false);
			} else if(error_blob) {
				const char *error = (const char *)error_blob->GetBufferPointer();
				Assert(false);
				error_blob->Release();
			}

			return 1;
		}

		result = device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), 0, &vertex_shader);
		Assert(SUCCEEDED(result));
	}

	ID3D11PixelShader *pixel_shader = 0;
	{
		ID3DBlob *ps_blob = 0;
		ID3DBlob *error_blob = 0;
		HRESULT result = D3DCompileFromFile(L"../shaders.hlsl", 0, 0, "ps_main", "ps_5_0", 0, 0, &ps_blob, &error_blob);
		if(FAILED(result)) {
			if(result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
				Assert(false);
			} else if(error_blob) {
				const char *error = (const char *)error_blob->GetBufferPointer();
				Assert(false);
				error_blob->Release();
			}

			return 1;
		}

		result = device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), 0, &pixel_shader);
		Assert(SUCCEEDED(result));

		ps_blob->Release();
	}

	ID3D11InputLayout *input_layout = 0;
	{
		D3D11_INPUT_ELEMENT_DESC input_element_desc[] = {
			{ "POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		HRESULT result = device->CreateInputLayout(input_element_desc,
												   ARRAYSIZE(input_element_desc),
												   vs_blob->GetBufferPointer(),
												   vs_blob->GetBufferSize(),
												   &input_layout);
		Assert(SUCCEEDED(result));
		vs_blob->Release();
	}

	ID3D11Buffer *vertex_buffer = 0;
	UINT vertex_n = 0;
	UINT stride = 0;
	UINT offset = 0;
	{
		float vertex_data[] = {
			 0.0f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f,
			 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f,
			-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f
		};

		stride = 6 * sizeof(float);
		vertex_n = sizeof(vertex_data) / stride;
		offset = 0;

		D3D11_BUFFER_DESC vertex_buffer_desc = {};
		vertex_buffer_desc.ByteWidth = sizeof(vertex_data);
		vertex_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
		vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA subresource_data = { vertex_data };

		HRESULT result = device->CreateBuffer(&vertex_buffer_desc, &subresource_data, &vertex_buffer);
		Assert(SUCCEEDED(result));
	}

	bool running = true;
	while(running) {
		MSG message = {};
		while(PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
			if(message.message == WM_QUIT) {
				running = false;
			}

			TranslateMessage(&message);
			DispatchMessageW(&message);
		}

		float background_color[4] = { 0.1f, 0.2f, 0.6f, 1.0f };
		device_context->ClearRenderTargetView(frame_buffer_view, background_color);

		RECT rect = {};
		GetClientRect(window, &rect);
		D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (float)(rect.right - rect.left), (float)(rect.bottom - rect.top), 0.0f, 1.0f };
		device_context->RSSetViewports(1, &viewport);

		device_context->OMSetRenderTargets(1, &frame_buffer_view, 0);

		device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		device_context->IASetInputLayout(input_layout);

		device_context->VSSetShader(vertex_shader, 0, 0);
		device_context->PSSetShader(pixel_shader, 0, 0);

		device_context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);

		device_context->Draw(vertex_n, 0);

		swap_chain->Present(1, 0);
	}

	return 0;
}