#include<Windows.h>
#include<d3d11.h>
#include"..\minhook\include\MinHook.h"
#pragma comment(lib,"d3d11.lib")

#ifdef _WIN64
#ifdef _DEBUG
#pragma comment(lib,"..\\minhook\\build\\VC15\\lib\\Debug\\libMinHook.x64.lib")
#else
#pragma comment(lib,"..\\minhook\\build\\VC15\\lib\\Release\\libMinHook.x64.lib")
#endif
#else
#ifdef _DEBUG
#pragma comment(lib,"..\\minhook\\build\\VC15\\lib\\Debug\\libMinHook.x86.lib")
#else
#pragma comment(lib,"..\\minhook\\build\\VC15\\lib\\Release\\libMinHook.x86.lib")
#endif
#endif

#include"custom_present.h"

typedef HRESULT(__stdcall*PFIDXGISwapChain_Present)(IDXGISwapChain*, UINT, UINT);
static PFIDXGISwapChain_Present pfPresent = nullptr, pfOriginalPresent = nullptr;

//Present��STDCALL���÷�ʽ��ֻ���THISָ����ڵ�һ��Ϳɰ��ǳ�Ա��������
HRESULT __stdcall HookedIDXGISwapChain_Present(IDXGISwapChain* p, UINT SyncInterval, UINT Flags)
{
	CustomPresent(p);
	//��ʱ���������أ�ֻ��ͨ��ָ����ã�����Ҫ�Ȱ�HOOK�رգ�����p->Present���ٿ���HOOK
	return pfOriginalPresent(p, SyncInterval, Flags);
}

PFIDXGISwapChain_Present GetPresentVAddr()
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
	//�������������DllMain�е��ã������¿�һ���߳�
	D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, device_flag, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION,
		&sc_desc, &pSC, &pDevice, &useLevel, &pContext);
	//��Ϊ��ͬ����麯���ǹ��õģ�����ֻ�贴��һ������Ķ���ͨ��ָ����ܻ�ȡ��������ַ
	//Present��VTable[8]��λ��
	INT_PTR p = reinterpret_cast<INT_PTR*>(reinterpret_cast<INT_PTR*>(pSC)[0])[8];
	pSC->Release();
	pContext->Release();
	pDevice->Release();
	return reinterpret_cast<PFIDXGISwapChain_Present>(p);
}

//�����Է�����û��DllMainʱ����
extern "C" __declspec(dllexport) BOOL StartHook()
{
	pfPresent = reinterpret_cast<PFIDXGISwapChain_Present>(GetPresentVAddr());
	if (MH_Initialize() != MH_OK)
		return FALSE;
	if (MH_CreateHook(pfPresent, HookedIDXGISwapChain_Present, reinterpret_cast<void**>(&pfOriginalPresent)) != MH_OK)
		return FALSE;
	if (MH_EnableHook(pfPresent) != MH_OK)
		return FALSE;
	return TRUE;
}

//�����Է�����û��DllMainʱ����
extern "C" __declspec(dllexport) BOOL StopHook()
{
	if (MH_DisableHook(pfPresent) != MH_OK)
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

//SetWindowHookEx��Ҫһ����������������DLL���ᱻ����
extern "C" __declspec(dllexport) LRESULT WINAPI HookProc(int code, WPARAM w, LPARAM l)
{
	return CallNextHookEx(NULL, code, w, l);
}
