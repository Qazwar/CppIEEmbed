#define STRICT
#define NOCRYPT
#pragma comment(lib,"MSVCRT.lib")
#include "webform.h"
#include "webwindow.h"
#include "webformdispatchimpl.h"
#include "jsobject.h"
#include <windows.h>
#include <tchar.h>
#include <map>
#include <string>
#include <sstream>
#include <cctype>
#include <algorithm>

#include <dwmapi.h>
#pragma comment(lib,"dwmapi.lib")

#include <vector>
#include <wininet.h>
#pragma comment(lib,"wininet.lib")
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>
#pragma comment (lib, "d2d1.lib")
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dx11.lib")
#pragma comment (lib, "d3dx10.lib")
// global declarations
IDXGISwapChain *swapchain;             // the pointer to the swap chain interface
ID3D11Device *dev;                     // the pointer to our Direct3D device interface
ID3D11DeviceContext *devcon;           // the pointer to our Direct3D device context
ID3D11RenderTargetView *backbuffer;    // the pointer to our back buffer
ID3D11Device *pDevice = NULL;
ID3D11DeviceContext *pContext = NULL;

int Width = GetSystemMetrics(SM_CXSCREEN);
int Height = GetSystemMetrics(SM_CYSCREEN);


bool loaded;
WebWindow* webWindow;
JSObject *jsobj;
WebformDispatchImpl *webformDispatchImpl;
HWND _hwnd;


void InitD3D(HWND hWnd)
{
	//init win hook

	DXGI_SWAP_CHAIN_DESC scd;

	// clear out the struct for use
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	// fill the swap chain description struct
	scd.BufferCount = 1;                                    // one back buffer
	scd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;     // use 32-bit color
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
	scd.OutputWindow = hWnd;                                // the window to be used
	scd.SampleDesc.Count = 1;                               // how many multisamples
	scd.SampleDesc.Quality = 0;                             // multisample quality level
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	scd.Windowed = TRUE;                                    // windowed/full-screen mode
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE; //MUST ADD THIS FLAG, PLEASE HACK IF NEEDED

	 // create a device, device context and swap chain using the information in the scd struct
	D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,

		NULL, //D3D11_CREATE_DEVICE_DEBUG,
		NULL,
		NULL,
		D3D11_SDK_VERSION,
		&scd,
		&swapchain,
		&dev,
		NULL,
		&devcon);


	// get the address of the back buffer
	ID3D11Texture2D *pBackBuffer;
	swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	swapchain->ResizeBuffers(1, Width, Height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);

	// use the back buffer address to create the render target
	dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
	pBackBuffer->Release();

	// set the render target as the back buffer
	devcon->OMSetRenderTargets(1, &backbuffer, NULL);


	// Set the viewport
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = Width;
	viewport.Height = Height;

	devcon->RSSetViewports(1, &viewport);
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	} break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}
ID3D11ShaderResourceView* pIcon = NULL;

void Render() {

	static bool firstTime = true;
	static IDXGISurface1 *g_pSurface1 = NULL;
	if (firstTime)
	{
		if (SUCCEEDED(swapchain->GetDevice(__uuidof(ID3D11Device), (void **)&pDevice))) {
			pDevice->GetImmediateContext(&pContext);
		}
		swapchain->GetBuffer(0, __uuidof(IDXGISurface1), (void**)&g_pSurface1); //Can get the value of IDXGISurface1 here
		firstTime = false;
	}

	//bitblt?

	//if (pIcon) {


		if (!webWindow->webForm->ibrowser || (webWindow->webForm->isnaving & 2)) {

		}
		else {
			HDC g_hDC;
			g_pSurface1->GetDC(TRUE, &g_hDC);
			IHTMLElement* e;
			webWindow->webForm->GetDoc()->get_body(&e);

			IHTMLElementRender* r;
			e->QueryInterface(IID_IHTMLElementRender, (void**)&r);
			r->DrawToDC(g_hDC);
			g_pSurface1->ReleaseDC(NULL);;
		}

}
// this is the function used to render a single frame
void RenderFrame(void)
{
	// clear the back buffer to a deep blue
	//devcon->ClearRenderTargetView(backbuffer, D3DXCOLOR(0.0f, 0.2f, 0.4f, 1.0f));
	devcon->ClearRenderTargetView(backbuffer, D3DXCOLOR(0.0f, 0.0f, 0.0f, 0.0f));

	// Set shader texture resource in the pixel shader.

	// do 3D rendering on the back buffer here
	Render();

	// switch the back buffer and the front buffer
	swapchain->Present(0, 0);
}


HHOOK _hookKb;
HHOOK _hookKb2;
struct _msg {
	HWND hwnd;
	DWORD code;
	WPARAM wParam;
	LONG x;
	LONG y;
};
std::vector<_msg> msgs;
LRESULT HookCallbackKb(int nCode, WPARAM wParam, LPARAM lParam)
{
	MSLLHOOKSTRUCT * pHookStruct = (MSLLHOOKSTRUCT *)lParam;
	msgs.push_back({ 0,(DWORD)wParam,pHookStruct->mouseData,pHookStruct->pt.x, pHookStruct->pt.y });
	return CallNextHookEx(_hookKb, nCode, wParam, lParam);
}
#include <thread>
LRESULT HookCallbackKb2(int nCode, WPARAM wParam, LPARAM lParam) {
	KBDLLHOOKSTRUCT* mhs = (KBDLLHOOKSTRUCT*)lParam;
	if(wParam != WM_KEYUP)
		msgs.push_back({ 0,(DWORD)wParam,mhs->vkCode,0,0 });
	return CallNextHookEx(_hookKb2, nCode, wParam, lParam);

}
DWORD WINAPI KbThread(LPVOID lParam) {
	_hookKb2 = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallbackKb2, NULL, 0);
	MSG msg;
	while (1) {//GetMessage(&msg,NULL,0,0)) {
		PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
		std::this_thread::sleep_for(std::chrono::nanoseconds(1));
	}
	return 0;
}
DWORD WINAPI MouseThread(LPVOID lParam) {
	//init win hook
		CreateThread(0, 0, KbThread, 0, 0, 0);
		_hookKb = SetWindowsHookEx(WH_MOUSE_LL, HookCallbackKb, NULL, 0);
	MSG msg;
	while (1) {
		PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE);
		std::this_thread::sleep_for(std::chrono::nanoseconds(1));
	}
	return 0;
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	DeleteUrlCacheEntry("http://127.0.0.1/test.html");
	OleInitialize(0);

	jsobj = new JSObject();
	jsobj->AddRef();
	webformDispatchImpl = new WebformDispatchImpl(jsobj);

	webWindow = new WebWindow(webformDispatchImpl);
	webWindow->Create(hInstance, 0, 0, Width, Height*2, true);
	//webWindow->webForm->Go("http://127.0.0.1/test.html");
	char path[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH,path);
	strcat_s(path, "\\canvas.html");
	webWindow->webForm->Go(path);
	ShowWindow(webWindow->hWndWebWindow, SW_HIDE);
	HWND hWnd;
	WNDCLASSEX wc;

	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "WindowClass";

	RegisterClassEx(&wc);
	RECT wr = { 0, 0, Width, Height };
	hWnd = CreateWindowExW(NULL,
		L"WindowClass",
		L"Our First Direct3D Program",
		WS_EX_TOPMOST | WS_EX_LAYERED,
		0,
		0,
		wr.right - wr.left,
		wr.bottom - wr.top,
		NULL,
		NULL,
		hInstance,
		NULL);
	_hwnd = hWnd;
	ShowWindow(hWnd, nCmdShow);
	SetMenu(_hwnd, NULL);
	SetWindowLongPtr(_hwnd, GWL_STYLE, WS_VISIBLE);
	SetWindowLongPtr(_hwnd, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT); 

	MARGINS margins = { -1 }; // With DWM
	DwmExtendFrameIntoClientArea(_hwnd, &margins);
	// set up and initialize Direct3D
	InitD3D(hWnd);
	SetWindowPos(_hwnd, HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_SHOWWINDOW);
	CreateThread(0, 0, MouseThread, 0, 0, 0);
	MSG msg;
	HWND xwnd;


	while (1) {

		if (PeekMessage(&msg, NULL, 0, 0, 1)) {
			if (msg.message == WM_QUIT) break;
			if (msg.wParam == 0x00001008) {
				//save hwnd
				xwnd = msg.hwnd;
			} 
			if (msg.message == WM_TIMER) { //WM_TIMER ignores setinterval. but also removes focus..
			} else if ( msg.message != WM_MOUSEMOVE) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		} else webWindow->webForm->RunJSFunction("Update();");
		while (!msgs.empty()) {
			auto m = msgs.back();
				msg = { xwnd,m.code,m.wParam,MAKELPARAM(m.x,m.y),0,{m.x,m.y} };
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			//}
			msgs.pop_back();
		}
		RenderFrame();
	}

	delete webWindow;
	delete webformDispatchImpl;
	jsobj->Release();

	OleUninitialize();
	return (int)msg.wParam;
}
