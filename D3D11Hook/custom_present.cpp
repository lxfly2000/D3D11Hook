#include"custom_present.h"
#include"ResLoader.h"
#include"..\DirectXTK\Inc\SimpleMath.h"
#include<map>

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
	TCHAR fpstext[16];
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
		C(pSC->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice));//�������Ҫ���ͷ�ʱ����Release
		resloader.Init(pDevice);
		resloader.SetSwapChain(pSC);
		pDevice->GetImmediateContext(&pContext);
		spriteBatch = std::make_unique<DirectX::SpriteBatch>(pContext);
		C(resloader.LoadFontFromSystem(spriteFont, 1024, 1024, L"����", 48, D2D1::ColorF(D2D1::ColorF::White), DWRITE_FONT_WEIGHT_REGULAR));
		textpos.x = textpos.y = 0.0f;
		textanchorpos.x = textanchorpos.y = 0.0f;
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
			fcount = 60;
			t1 = t2;
			t2 = GetTickCount();
			if (t1 == t2)
				t1--;
			wsprintf(fpstext, TEXT("FPS:%3d"), 60000 / (t2 - t1));//ע��wsprintf��֧�ָ�������ʽ��
		}
		//ʹ��SpriteBatch���ƻ�֮ǰ����Ⱦ��״̬���Ҳ����Զ�����ͻָ�ԭ״̬����ͼǰӦ�ȱ���ԭ����״̬����ɺ�ָ�
		//�ο���https://github.com/Microsoft/DirectXTK/wiki/SpriteBatch#state-management
		//https://github.com/ocornut/imgui/blob/master/examples/imgui_impl_dx11.cpp#L130
		//����д����һ�����������ʱ����ֻҪ����Hook�󲻹��Ƿ�ֹͣ��û���ٻ����������ˣ������е���Դ���޷��ָ��İ�
#pragma region ��ȡԭ����״̬
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
#pragma region ��SpriteBatch����
		spriteBatch->Begin();
		spriteFont->DrawString(spriteBatch.get(), fpstext, DirectX::SimpleMath::Vector2(textpos.x + 2.0f, textpos.y + 2.0f), DirectX::Colors::Gray, 0.0f, textanchorpos);
		spriteFont->DrawString(spriteBatch.get(), fpstext, textpos, DirectX::Colors::White, 0.0f, textanchorpos);
		spriteBatch->End();
#pragma endregion
#pragma region �ָ�ԭ����״̬
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