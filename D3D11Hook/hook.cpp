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

static int SpeedGear_frameCounter = 0;
#include <d3dkmthk.h>
D3DKMT_WAITFORVERTICALBLANKEVENT getVBlankHandle() {
	//https://docs.microsoft.com/en-us/windows/desktop/gdi/getting-information-on-a-display-monitor
	DISPLAY_DEVICE dd;
	dd.cb = sizeof(DISPLAY_DEVICE);

	DWORD deviceNum = 0;
	while (EnumDisplayDevices(NULL, deviceNum, &dd, 0)) {
		if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
			break;
		/*
		DISPLAY_DEVICE newdd = {0};
		newdd.cb = sizeof(DISPLAY_DEVICE);
		DWORD monitorNum = 0;
		while (EnumDisplayDevices(dd.DeviceName, monitorNum, &newdd, 0)) {
			DumpDevice(newdd, 4);
			monitorNum++;
		}
		*/
		deviceNum++;
	}

	HDC hdc = CreateDC(NULL, dd.DeviceName, NULL, NULL);

	D3DKMT_OPENADAPTERFROMHDC OpenAdapterData;

	OpenAdapterData.hDc = hdc;
	if (0 == D3DKMTOpenAdapterFromHdc(&OpenAdapterData)) {
		DeleteDC(hdc);
	}
	else {
		DeleteDC(hdc);
	}
	D3DKMT_WAITFORVERTICALBLANKEVENT we;
	we.hAdapter = OpenAdapterData.hAdapter;
	we.hDevice = 0; //optional. maybe OpenDeviceHandle will give it to us, https://docs.microsoft.com/en-us/windows/desktop/api/dxva2api/nf-dxva2api-idirect3ddevicemanager9-opendevicehandle
	we.VidPnSourceId = OpenAdapterData.VidPnSourceId;

	return we;
}

D3DKMT_WAITFORVERTICALBLANKEVENT wv;
bool wvget = true;

HRESULT hrLastPresent = S_OK;

//Present��STDCALL���÷�ʽ��ֻ���THISָ����ڵ�һ��Ϳɰ��ǳ�Ա��������
HRESULT __stdcall HookedIDXGISwapChain_Present(IDXGISwapChain* p, UINT SyncInterval, UINT Flags)
{
	CustomPresent(p);
	//��ʱ���������أ�ֻ��ͨ��ָ����ã�����Ҫ�Ȱ�HOOK�رգ�����p->Present���ٿ���HOOK
	if (SpeedGear::GetCurrentSpeed() >= 1.0f)
	{
		if (SpeedGear_frameCounter == 0)
			hrLastPresent = pfOriginalPresent(p,SyncInterval,Flags);
		SpeedGear_frameCounter = (SpeedGear_frameCounter + 1) % static_cast<int>(SpeedGear::GetCurrentSpeed());
	}
	else
	{
		hrLastPresent = pfOriginalPresent(p, SyncInterval, Flags);
		if (wvget)
		{
			wv = getVBlankHandle();
			wvget = false;
		}
		for (int i = 0; i < (int)(1.0f / SpeedGear::GetCurrentSpeed()); i++)
			D3DKMTWaitForVerticalBlankEvent(&wv);
	}
	return hrLastPresent;
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
	//�������������DllMain�е��ã������¿�һ���߳�
	D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, device_flag, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION,
		&sc_desc, &pSC, &pDevice, &useLevel, &pContext);
	//��Ϊ��ͬ����麯���ǹ��õģ�����ֻ�贴��һ������Ķ���ͨ��ָ����ܻ�ȡ��������ַ
	//Present��VTable[8]��λ��
	INT_PTR pP = reinterpret_cast<INT_PTR*>(reinterpret_cast<INT_PTR*>(pSC)[0])[8];
	INT_PTR pRB = reinterpret_cast<INT_PTR*>(reinterpret_cast<INT_PTR*>(pSC)[0])[13];
	*pPresent=reinterpret_cast<PFIDXGISwapChain_Present>(pP);
	*pResizeBuffers = reinterpret_cast<PFIDXGISwapChain_ResizeBuffers>(pRB);
	pSC->Release();
	pContext->Release();
	pDevice->Release();
}

//�����Է�����û��DllMainʱ����
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

//�����Է�����û��DllMainʱ����
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

extern "C" __declspec(dllexport) BOOL SpeedGear_StartHook()
{
	return SpeedGear::InitCustomTime();
}

extern "C" __declspec(dllexport) BOOL SpeedGear_StopHook()
{
	return SpeedGear::UninitCustomTime();
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
		SpeedGear_StartHook();//��Ϣ���Ӳ��ܵ������߳�ʹ��
		CreateThread(NULL, 0, TInitHook, NULL, 0, NULL);
		break;
	case DLL_PROCESS_DETACH:
		StopHook();
		SpeedGear_StopHook();
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
