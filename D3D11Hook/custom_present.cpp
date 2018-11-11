#include"custom_present.h"
#include"ResLoader.h"
#include"..\DirectXTK\Inc\SimpleMath.h"

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
public:
	bool needinit;
	D2DCustomPresent():needinit(true)
	{
	}
	~D2DCustomPresent()
	{
		resloader.Uninit();
	}
	BOOL Init(IDXGISwapChain*pSC)
	{
		ID3D11Device *pDevice;
		C(pSC->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice));
		resloader.Init(pDevice);
		resloader.SetSwapChain(pSC);
		ID3D11DeviceContext *pContext;
		pDevice->GetImmediateContext(&pContext);
		spriteBatch = std::make_unique<DirectX::SpriteBatch>(pContext);
		C(resloader.LoadFontFromSystem(spriteFont, 1024, 1024, L"宋体", 48, D2D1::ColorF(D2D1::ColorF::White), DWRITE_FONT_WEIGHT_REGULAR));
		textpos.x = textpos.y = 0.0f;
		textanchorpos.x = textanchorpos.y = 0.0f;
		return TRUE;
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
		spriteBatch->Begin();
		spriteFont->DrawString(spriteBatch.get(), fpstext, textpos, DirectX::Colors::White, 0.0f, textanchorpos);
		spriteBatch->End();
	}
};

static D2DCustomPresent cp;

void CustomPresent(IDXGISwapChain *pSC)
{
	if (cp.needinit)
	{
		cp.needinit = false;
		cp.Init(pSC);
	}
	cp.Draw();
}