#include<Windows.h>
#include<d3d11.h>
#include"..\minhook\include\MinHook.h"
#pragma comment(lib,"d3d11.lib")

#include"custom_present.h"

typedef HRESULT(__stdcall*PFIDXGISwapChain_Present)(IDXGISwapChain*, UINT, UINT);
typedef HRESULT(__stdcall* PFIDXGISwapChain_ResizeBuffers)(IDXGISwapChain*,UINT, UINT, UINT, DXGI_FORMAT, UINT);
static PFIDXGISwapChain_Present pfPresent = nullptr, pfOriginalPresent = nullptr;
static PFIDXGISwapChain_ResizeBuffers pfResizeBuffers = nullptr, pfOriginalResizeBuffers = nullptr;
static HMODULE hDllModule;

DWORD GetDLLPath(LPTSTR path, DWORD max_length)
{
	return GetModuleFileName(hDllModule, path, max_length);
}

//Present是STDCALL调用方式，只需把THIS指针放在第一项就可按非成员函数调用
HRESULT __stdcall HookedIDXGISwapChain_Present(IDXGISwapChain* p, UINT SyncInterval, UINT Flags)
{
	CustomPresent(p);
	//此时函数被拦截，只能通过指针调用，否则要先把HOOK关闭，调用p->Present，再开启HOOK
	return pfOriginalPresent(p, SyncInterval, Flags);
}

HRESULT __stdcall HookedIDXGISwapChain_ResizeBuffers(IDXGISwapChain*p,UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	CustomResizeBuffers(p,BufferCount, Width, Height, NewFormat, SwapChainFlags);
	return pfOriginalResizeBuffers(p, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

void GetPresentVAddr(PFIDXGISwapChain_Present*pPresent,PFIDXGISwapChain_ResizeBuffers*pResizeBuffers)
{
	ID3D11Device *pDevice;
	D3D_FEATURE_LEVEL useLevel;
	ID3D11DeviceContext *pContext;
	IDXGISwapChain *pSC;
	DXGI_SWAP_CHAIN_DESC sc_desc;
	ZeroMemory(&sc_desc, sizeof sc_desc);
	sc_desc.BufferCount = 1;
	sc_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sc_desc.BufferDesc.Width = 800;
	sc_desc.BufferDesc.Height = 600;
	sc_desc.BufferDesc.RefreshRate.Denominator = 1;
	sc_desc.BufferDesc.RefreshRate.Numerator = 60;
	sc_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sc_desc.OutputWindow = GetDesktopWindow();
	sc_desc.SampleDesc.Count = 1;
	sc_desc.SampleDesc.Quality = 0;
	sc_desc.Windowed = TRUE;
	UINT device_flag =
#ifdef _DEBUG
		D3D11_CREATE_DEVICE_DEBUG;
#else
		0;
#endif
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_10_1,D3D_FEATURE_LEVEL_10_0 };
	//这个函数不能在DllMain中调用，必须新开一个线程
	D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, device_flag, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION,
		&sc_desc, &pSC, &pDevice, &useLevel, &pContext);
	//因为相同类的虚函数是共用的，所以只需创建一个该类的对象，通过指针就能获取到函数地址
	//Present在VTable[8]的位置
	INT_PTR pP = reinterpret_cast<INT_PTR*>(reinterpret_cast<INT_PTR*>(pSC)[0])[8];
	INT_PTR pRB = reinterpret_cast<INT_PTR*>(reinterpret_cast<INT_PTR*>(pSC)[0])[13];
	*pPresent=reinterpret_cast<PFIDXGISwapChain_Present>(pP);
	*pResizeBuffers = reinterpret_cast<PFIDXGISwapChain_ResizeBuffers>(pRB);
	pSC->Release();
	pContext->Release();
	pDevice->Release();
}

//导出以方便在没有DllMain时调用
extern "C" __declspec(dllexport) BOOL StartHook()
{
	GetPresentVAddr(&pfPresent,&pfResizeBuffers);
	if (MH_Initialize() != MH_OK)
		return FALSE;
	if (MH_CreateHook(pfPresent, HookedIDXGISwapChain_Present, reinterpret_cast<void**>(&pfOriginalPresent)) != MH_OK)
		return FALSE;
	if (MH_CreateHook(pfResizeBuffers, HookedIDXGISwapChain_ResizeBuffers, reinterpret_cast<void**>(&pfOriginalResizeBuffers)) != MH_OK)
		return FALSE;
	if (MH_EnableHook(pfPresent) != MH_OK)
		return FALSE;
	if (MH_EnableHook(pfResizeBuffers) != MH_OK)
		return FALSE;
	return TRUE;
}

//导出以方便在没有DllMain时调用
extern "C" __declspec(dllexport) BOOL StopHook()
{
	if (MH_DisableHook(pfResizeBuffers) != MH_OK)
		return FALSE;
	if (MH_DisableHook(pfPresent) != MH_OK)
		return FALSE;
	if (MH_RemoveHook(pfResizeBuffers) != MH_OK)
		return FALSE;
	if (MH_RemoveHook(pfPresent) != MH_OK)
		return FALSE;
	if (MH_Uninitialize() != MH_OK)
		return FALSE;
	return TRUE;
}

DWORD WINAPI TInitHook(LPVOID param)
{
	return StartHook();
}

BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved)
{
	hDllModule = hInstDll;
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hInstDll);
		CreateThread(NULL, 0, TInitHook, NULL, 0, NULL);
		break;
	case DLL_PROCESS_DETACH:
		StopHook();
		break;
	case DLL_THREAD_ATTACH:break;
	case DLL_THREAD_DETACH:break;
	}
	return TRUE;
}

//SetWindowHookEx需要一个导出函数，否则DLL不会被加载
extern "C" __declspec(dllexport) LRESULT WINAPI HookProc(int code, WPARAM w, LPARAM l)
{
	return CallNextHookEx(NULL, code, w, l);
}
