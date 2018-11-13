#include"custom_present.h"
#include"ResLoader.h"
#include"..\DirectXTK\Inc\SimpleMath.h"
#include<map>

#ifdef _DEBUG
#define C(x) if(FAILED(x)){MessageBox(NULL,TEXT(_CRT_STRINGIZE(x)),NULL,MB_ICONERROR);throw E_FAIL;}
#else
#define C(x) x
#endif

//https://stackoverflow.com/a/13438807
BOOL CheckWindowsVersion(DWORD dwMajor, DWORD dwMinor, DWORD dwBuild)
{
	// Initialize the OSVERSIONINFOEX structure.
	OSVERSIONINFOEX osvi;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwMajorVersion = dwMajor;
	osvi.dwMinorVersion = dwMinor;
	osvi.dwBuildNumber = dwBuild;

	// Initialize the condition mask.
	DWORDLONG dwlConditionMask = 0;
	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_BUILDNUMBER, VER_GREATER_EQUAL);

	// Perform the test.
	return VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, dwlConditionMask);
}

class D2DCustomPresent
{
private:
	std::unique_ptr<DirectX::SpriteBatch> spriteBatch;
	std::unique_ptr<DirectX::SpriteFont> spriteFont;
	DirectX::SimpleMath::Vector2 textpos, textanchorpos;
	ResLoader resloader;
	ID3D11DeviceContext *pContext;
	NOTIFYICONDATA nid;
public:
	D2DCustomPresent():pContext(nullptr)
	{
		nid.hWnd = NULL;
	}
	D2DCustomPresent(D2DCustomPresent &&other)
	{
		spriteFont = std::move(other.spriteFont);
		spriteFont = std::move(other.spriteFont);
		textpos = std::move(other.textpos);
		textanchorpos = std::move(other.textanchorpos);
		resloader = std::move(other.resloader);
		pContext = std::move(other.pContext);
		nid = std::move(other.nid);
	}
	~D2DCustomPresent()
	{
		Uninit();
		if (nid.hWnd)
			Shell_NotifyIcon(NIM_DELETE, &nid);
	}
	void ShowTrayBalloon(IDXGISwapChain *pSC)
	{
		if (nid.hWnd)
			Shell_NotifyIcon(NIM_DELETE, &nid);
		DXGI_SWAP_CHAIN_DESC desc;
		pSC->GetDesc(&desc);
		nid.hWnd = desc.OutputWindow;
		if (CheckWindowsVersion(6, 0, 6))
			nid.cbSize = sizeof(NOTIFYICONDATA);
		else if (CheckWindowsVersion(6, 0, 0))
			nid.cbSize = NOTIFYICONDATA_V3_SIZE;
		else if (CheckWindowsVersion(5, 0, 0))
			nid.cbSize = NOTIFYICONDATA_V2_SIZE;
		else
			nid.cbSize = NOTIFYICONDATA_V1_SIZE;
		nid.uID = 2000;
		nid.hIcon = (HICON)GetClassLongPtr(nid.hWnd, GCLP_HICON);
		nid.uFlags = NIF_ICON | NIF_TIP | NIF_INFO;
		nid.uVersion = NOTIFYICON_VERSION;
		nid.uTimeout = 5000;
		TCHAR text[256];
		GetWindowText(nid.hWnd, text, ARRAYSIZE(text));
		wsprintf(nid.szTip, TEXT("D3D11Hook [%s]"), text);
		lstrcpy(nid.szInfoTitle, TEXT("D3D11Hook 加载成功。"));
		wsprintf(nid.szInfo, TEXT("SwapChain: %p\nHWND: %p"), pSC, nid.hWnd);
		nid.dwInfoFlags = NIIF_INFO;
		Shell_NotifyIcon(NIM_ADD, &nid);
	}
	BOOL Init(IDXGISwapChain*pSC)
	{
		ID3D11Device *pDevice;
		C(pSC->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice));//这个不需要在释放时调用Release
		resloader.Init(pDevice);
		resloader.SetSwapChain(pSC);
		pDevice->GetImmediateContext(&pContext);
		spriteBatch = std::make_unique<DirectX::SpriteBatch>(pContext);
		C(resloader.LoadFontFromSystem(spriteFont, 1024, 1024, L"宋体", 48, D2D1::ColorF(D2D1::ColorF::White), DWRITE_FONT_WEIGHT_REGULAR));
		textpos.x = textpos.y = 0.0f;
		textanchorpos.x = textanchorpos.y = 0.0f;
		ShowTrayBalloon(pSC);
		return TRUE;
	}
	void Uninit()
	{
		if(pContext)pContext->Release();
		resloader.Uninit();
	}
	void Draw()
	{
		static unsigned t1 = 0, t2 = 0, fcount = 0;
		static TCHAR fpstext[16];
		if (fcount--==0)
		{
			fcount = 60;
			t1 = t2;
			t2 = GetTickCount();
			if (t1 == t2)
				t1--;
			wsprintf(fpstext, TEXT("FPS:%3d"), 60000 / (t2 - t1));//注意wsprintf不支持浮点数格式化
		}
		//使用SpriteBatch会破坏之前的渲染器状态并且不会自动保存和恢复原状态，画图前应先保存原来的状态，完成后恢复
		//参考：https://github.com/Microsoft/DirectXTK/wiki/SpriteBatch#state-management
#pragma region 获取原来的状态
		ID3D11BlendState *blendState; FLOAT blendFactor[4]; UINT sampleMask;
		ID3D11SamplerState *samplerStateVS0;
		ID3D11DepthStencilState *depthStencilState; UINT stencilRef;
		ID3D11Buffer *indexBuffer; DXGI_FORMAT indexBufferFormat; UINT indexBufferOffset;
		//ID3D11InputLayout *inputLayout;
		//ID3D11PixelShader *pixelShader; ID3D11ClassInstance *psClassInstances; UINT psNClassInstances;
		D3D11_PRIMITIVE_TOPOLOGY primitiveTopology;
		ID3D11RasterizerState *rasterState;
		//ID3D11SamplerState *samplerStatePS0;
		ID3D11ShaderResourceView *resourceViewPS0;
		ID3D11Buffer *vb0; UINT stridesVB0, offsetVB0;
		//ID3D11VertexShader *vertexShader; ID3D11ClassInstance *vsClassInstances; UINT vsNClassInstances;
		pContext->OMGetBlendState(&blendState, blendFactor, &sampleMask);
		pContext->VSGetSamplers(0, 1, &samplerStateVS0);
		pContext->OMGetDepthStencilState(&depthStencilState, &stencilRef);
		pContext->IAGetIndexBuffer(&indexBuffer, &indexBufferFormat, &indexBufferOffset);
		//pContext->IAGetInputLayout(&inputLayout);
		//pContext->PSGetShader(&pixelShader, &psClassInstances, &psNClassInstances);
		pContext->IAGetPrimitiveTopology(&primitiveTopology);
		pContext->RSGetState(&rasterState);
		//pContext->PSGetSamplers(0, 1, &samplerStatePS0);
		pContext->PSGetShaderResources(0, 1, &resourceViewPS0);
		pContext->IAGetVertexBuffers(0, 1, &vb0, &stridesVB0, &offsetVB0);
		//pContext->VSGetShader(&vertexShader, &vsClassInstances, &vsNClassInstances);
#pragma endregion
#pragma region 用SpriteBatch绘制
		spriteBatch->Begin();
		spriteFont->DrawString(spriteBatch.get(), fpstext, DirectX::SimpleMath::Vector2(textpos.x + 2.0f, textpos.y + 2.0f), DirectX::Colors::Gray, 0.0f, textanchorpos);
		spriteFont->DrawString(spriteBatch.get(), fpstext, textpos, DirectX::Colors::White, 0.0f, textanchorpos);
		spriteBatch->End();
#pragma endregion
#pragma region 恢复原来的状态
		pContext->OMSetBlendState(blendState, blendFactor, sampleMask);
		pContext->VSSetSamplers(0, 1, &samplerStateVS0);
		pContext->OMSetDepthStencilState(depthStencilState, stencilRef);
		pContext->IASetIndexBuffer(indexBuffer, indexBufferFormat, indexBufferOffset);
		//pContext->IASetInputLayout(inputLayout);
		//pContext->PSSetShader(pixelShader, &psClassInstances, psNClassInstances);
		pContext->IASetPrimitiveTopology(primitiveTopology);
		pContext->RSSetState(rasterState);
		//pContext->PSSetSamplers(0, 1, &samplerStatePS0);
		pContext->PSSetShaderResources(0, 1, &resourceViewPS0);
		pContext->IASetVertexBuffers(0, 1, &vb0, &stridesVB0, &offsetVB0);
		//pContext->VSSetShader(vertexShader, &vsClassInstances, vsNClassInstances);
		if(blendState)blendState->Release();
		if(samplerStateVS0)samplerStateVS0->Release();
		if(depthStencilState)depthStencilState->Release();
		if(indexBuffer)indexBuffer->Release();
		//if(inputLayout)inputLayout->Release();
		//if(pixelShader)pixelShader->Release(); if(psClassInstances)psClassInstances->Release();
		if(rasterState)rasterState->Release();
		//if(samplerStatePS0)samplerStatePS0->Release();
		if(resourceViewPS0)resourceViewPS0->Release();
		if(vb0)vb0->Release();
		//if(vertexShader)vertexShader->Release(); if(vsClassInstances)vsClassInstances->Release();
#pragma endregion
	}
};

static std::map<IDXGISwapChain*, D2DCustomPresent> cp;

void CustomPresent(IDXGISwapChain *pSC)
{
	if (cp.find(pSC) == cp.end())
	{
		cp.insert(std::make_pair(pSC, D2DCustomPresent()));
		cp[pSC].Init(pSC);
	}
	cp[pSC].Draw();
}