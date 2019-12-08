#pragma once
#include<dxgi.h>
#ifdef __cplusplus
extern "C" {
#endif
//自定义Present的附加操作
void CustomPresent(IDXGISwapChain*);
DWORD GetDLLPath(LPTSTR path, DWORD max_length);
#ifdef __cplusplus
}
#endif
