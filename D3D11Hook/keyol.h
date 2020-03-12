#pragma once
#include<Windows.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

LRESULT CALLBACK KeyOverlayExtraProcess(WNDPROC oldProc, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL KeyOverlayInit(HWND hwnd, ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void KeyOverlayUninit();
void KeyOverlayDraw();
