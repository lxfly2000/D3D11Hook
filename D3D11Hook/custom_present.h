#pragma once
#include<dxgi.h>
#ifdef __cplusplus
extern "C" {
#endif
//自定义Present的附加操作
void CustomPresent(IDXGISwapChain*);
#ifdef __cplusplus
}
#endif
