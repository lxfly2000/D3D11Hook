#pragma once
#include<dxgi.h>
#ifdef __cplusplus
extern "C" {
#endif
//�Զ���Present�ĸ��Ӳ���
void CustomPresent(IDXGISwapChain*);
DWORD GetDLLPath(LPTSTR path, DWORD max_length);
#ifdef __cplusplus
}
#endif
