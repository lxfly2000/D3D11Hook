#include"custom_present.h"
#include"ResLoader.h"
#include"..\DirectXTK\Inc\SimpleMath.h"
#include<map>
#include<string>
#include<ctime>

#ifdef _DEBUG
#define C(x) if(FAILED(x)){MessageBox(NULL,TEXT(_CRT_STRINGIZE(x)),NULL,MB_ICONERROR);throw E_FAIL;}
#else
#define C(x) x
#endif

class D2DCustomPresent
{
private:
	std::unique_ptr<DirectX::SpriteBatch> spriteBatch;
	std::unique_ptr<DirectX::SpriteFont> spriteFont;
	DirectX::SimpleMath::Vector2 textpos, textanchorpos;
	ResLoader resloader;
	ID3D11DeviceContext *pContext;
	unsigned t1, t2, fcount;
	std::wstring display_text;
	int current_fps;
	TCHAR time_text[32], fps_text[32];

	TCHAR font_name[256], font_size[16],text_x[16],text_y[16],text_anchor_x[16],text_anchor_y[16],display_text_fmt[256],fps_fmt[32],time_fmt[32];
	TCHAR font_red[16], font_green[16], font_blue[16], font_alpha[16];
	TCHAR font_shadow_red[16], font_shadow_green[16], font_shadow_blue[16], font_shadow_alpha[16], font_shadow_distance[16];
	int font_weight,period_frames;
	DirectX::XMVECTOR calcColor, calcShadowColor;
	DirectX::XMFLOAT2 calcShadowPos;
public:
	D2DCustomPresent():pContext(nullptr),t1(0),t2(0),fcount(0)
	{
	}
	D2DCustomPresent(D2DCustomPresent &&other)
	{
		spriteBatch = std::move(other.spriteBatch);
		spriteFont = std::move(other.spriteFont);
		textpos = std::move(other.textpos);
		textanchorpos = std::move(other.textanchorpos);
		resloader = std::move(other.resloader);
		pContext = std::move(other.pContext);
		t1 = std::move(other.t1);
		t2 = std::move(other.t2);
		fcount = std::move(other.fcount);
	}
	~D2DCustomPresent()
	{
		Uninit();
	}
	BOOL Init(IDXGISwapChain*pSC)
	{
		ID3D11Device *pDevice;
		C(pSC->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice));//这个不需要在释放时调用Release
		resloader.Init(pDevice);
		resloader.SetSwapChain(pSC);
		pDevice->GetImmediateContext(&pContext);
		spriteBatch = std::make_unique<DirectX::SpriteBatch>(pContext);

		TCHAR szConfPath[MAX_PATH];
		GetDLLPath(szConfPath, ARRAYSIZE(szConfPath));
		lstrcpy(wcsrchr(szConfPath, '.'), TEXT(".ini"));
#define GetInitConfStr(key,def) GetPrivateProfileString(TEXT("Init"),TEXT(_STRINGIZE(key)),def,key,ARRAYSIZE(key),szConfPath)
#define GetInitConfInt(key,def) key=GetPrivateProfileInt(TEXT("Init"),TEXT(_STRINGIZE(key)),def,szConfPath)
#define F(_i_str) (float)_wtof(_i_str)
		GetInitConfStr(font_name, TEXT("宋体"));
		GetInitConfStr(font_size, TEXT("48"));
		GetInitConfStr(font_red, TEXT("1"));
		GetInitConfStr(font_green, TEXT("1"));
		GetInitConfStr(font_blue, TEXT("0"));
		GetInitConfStr(font_alpha, TEXT("1"));
		GetInitConfStr(font_shadow_red, TEXT("0.5"));
		GetInitConfStr(font_shadow_green, TEXT("0.5"));
		GetInitConfStr(font_shadow_blue, TEXT("0"));
		GetInitConfStr(font_shadow_alpha, TEXT("1"));
		GetInitConfStr(font_shadow_distance, TEXT("2"));
		GetInitConfInt(font_weight, 400);
		GetInitConfStr(text_x, TEXT("0"));
		GetInitConfStr(text_y, TEXT("0"));
		GetInitConfStr(text_anchor_x, TEXT("0"));
		GetInitConfStr(text_anchor_y, TEXT("0"));
		GetInitConfInt(period_frames, 60);
		GetInitConfStr(time_fmt, TEXT("%H:%M:%S"));
		GetInitConfStr(fps_fmt, TEXT("FPS:%3d"));
		GetInitConfStr(display_text_fmt, TEXT("{fps}"));

		C(resloader.LoadFontFromSystem(spriteFont, 1024, 1024, font_name, F(font_size), D2D1::ColorF(D2D1::ColorF::White), (DWRITE_FONT_WEIGHT)font_weight));
		textpos.x = F(text_x);
		textpos.y = F(text_y);
		textanchorpos.x = F(text_anchor_x);
		textanchorpos.y = F(text_anchor_y);
		calcShadowPos = DirectX::SimpleMath::Vector2(textpos.x + F(font_shadow_distance), textpos.y + F(font_shadow_distance));
		calcColor = DirectX::XMLoadFloat4(&DirectX::XMFLOAT4(F(font_red), F(font_green), F(font_blue), F(font_alpha)));
		calcShadowColor = DirectX::XMLoadFloat4(&DirectX::XMFLOAT4(F(font_shadow_red), F(font_shadow_green), F(font_shadow_blue), F(font_shadow_alpha)));
		return TRUE;
	}
	void Uninit()
	{
		if(pContext)pContext->Release();
		resloader.Uninit();
	}
	void Draw()
	{
		if (fcount--==0)
		{
			fcount = period_frames;
			t1 = t2;
			t2 = GetTickCount();
			if (t1 == t2)
				t1--;
			current_fps = period_frames * 1000 / (t2 - t1);
			wsprintf(fps_text, fps_fmt, current_fps);//注意wsprintf不支持浮点数格式化
			time_t t1 = time(NULL);
			tm tm1;
			localtime_s(&tm1, &t1);
			wcsftime(time_text, ARRAYSIZE(time_text), time_fmt, &tm1);
			display_text = display_text_fmt;
			size_t pos = display_text.find(TEXT("\\n"));
			if (pos != std::wstring::npos)
				display_text.replace(pos, 2, TEXT("\n"));
			pos = display_text.find(TEXT("{fps}"));
			if (pos != std::wstring::npos)
				display_text.replace(pos, 5, fps_text);
			pos = display_text.find(TEXT("{time}"));
			if (pos != std::wstring::npos)
				display_text.replace(pos, 6, time_text);
		}
		//使用SpriteBatch会破坏之前的渲染器状态并且不会自动保存和恢复原状态，画图前应先保存原来的状态，完成后恢复
		//参考：https://github.com/Microsoft/DirectXTK/wiki/SpriteBatch#state-management
		//https://github.com/ocornut/imgui/blob/master/examples/imgui_impl_dx11.cpp#L130
		//在我写的另一个程序里测试时发现只要运行Hook后不管是否停止都没法再画出三角形了，可能有的资源是无法恢复的吧
#pragma region 获取原来的状态
		ID3D11BlendState *blendState; FLOAT blendFactor[4]; UINT sampleMask;
		ID3D11SamplerState *samplerStateVS0;
		ID3D11DepthStencilState *depthStencilState; UINT stencilRef;
		ID3D11Buffer *indexBuffer; DXGI_FORMAT indexBufferFormat; UINT indexBufferOffset;
		ID3D11InputLayout *inputLayout;
		ID3D11PixelShader *pixelShader; ID3D11ClassInstance *psClassInstances[256]; UINT psNClassInstances=256;
		D3D11_PRIMITIVE_TOPOLOGY primitiveTopology;
		ID3D11RasterizerState *rasterState;
		ID3D11SamplerState *samplerStatePS0;
		ID3D11ShaderResourceView *resourceViewPS0;
		ID3D11Buffer *vb0; UINT stridesVB0, offsetVB0;
		ID3D11VertexShader *vertexShader; ID3D11ClassInstance *vsClassInstances[256]; UINT vsNClassInstances=256;
		pContext->OMGetBlendState(&blendState, blendFactor, &sampleMask);
		pContext->VSGetSamplers(0, 1, &samplerStateVS0);
		pContext->OMGetDepthStencilState(&depthStencilState, &stencilRef);
		pContext->IAGetIndexBuffer(&indexBuffer, &indexBufferFormat, &indexBufferOffset);
		pContext->IAGetInputLayout(&inputLayout);//Need check
		pContext->PSGetShader(&pixelShader, psClassInstances, &psNClassInstances);//Need check
		pContext->IAGetPrimitiveTopology(&primitiveTopology);
		pContext->RSGetState(&rasterState);
		pContext->PSGetSamplers(0, 1, &samplerStatePS0);//Need check
		pContext->PSGetShaderResources(0, 1, &resourceViewPS0);
		pContext->IAGetVertexBuffers(0, 1, &vb0, &stridesVB0, &offsetVB0);
		pContext->VSGetShader(&vertexShader, vsClassInstances, &vsNClassInstances);//Need check
#pragma endregion
#pragma region 用SpriteBatch绘制
		spriteBatch->Begin();
		spriteFont->DrawString(spriteBatch.get(), display_text.c_str(), calcShadowPos, calcShadowColor, 0.0f, textanchorpos);
		spriteFont->DrawString(spriteBatch.get(), display_text.c_str(), textpos, calcColor, 0.0f, textanchorpos);
		spriteBatch->End();
#pragma endregion
#pragma region 恢复原来的状态
		pContext->OMSetBlendState(blendState, blendFactor, sampleMask);
		pContext->VSSetSamplers(0, 1, &samplerStateVS0);
		pContext->OMSetDepthStencilState(depthStencilState, stencilRef);
		pContext->IASetIndexBuffer(indexBuffer, indexBufferFormat, indexBufferOffset);
		pContext->IASetInputLayout(inputLayout);
		pContext->PSSetShader(pixelShader, psClassInstances, psNClassInstances);
		pContext->IASetPrimitiveTopology(primitiveTopology);
		pContext->RSSetState(rasterState);
		pContext->PSSetSamplers(0, 1, &samplerStatePS0);
		pContext->PSSetShaderResources(0, 1, &resourceViewPS0);
		pContext->IASetVertexBuffers(0, 1, &vb0, &stridesVB0, &offsetVB0);
		pContext->VSSetShader(vertexShader, vsClassInstances, vsNClassInstances);
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