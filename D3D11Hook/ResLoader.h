#pragma once
#include <dwrite.h>
#include <d2d1.h>
#include <wrl\client.h>
#include "..\DirectXTK\Inc\SpriteFont.h"

//����������������D3D��������������������
//���ļ����ز���ͼƬ
HRESULT LoadTextureFromFile(ID3D11Device*device,LPWSTR fpath, ID3D11ShaderResourceView **pTex, int *pw, int *ph,
	bool convertpmalpha = true);
//COM����������ϵͳ�е����嵽�������Ҫ�ֶ��ͷ�(delete)
//pCharacters��ָ��ʹ�õ��ַ���UTF16LE���룬�����ָ����ΪASCII�ַ���32��127��
//ע���ַ������뱣�����򣬱��뺬������ַ���Ĭ��Ϊ��?����������SetDefaultCharacter�޸ģ���
//�����С�ĵ�λΪͨ��������ֺ�
//pxBetweenChar��ָʾ�ַ���ˮƽ+��ֱ��࣬�ò�����Ϊ�˱�����Ƴ����ַ��غ϶����õģ�����Ӱ��ʵ����ʾ�ļ�ࡣ
//��ע�⡿���������ع��ܴ����ַ�λ��ƫ�Ƶ�Bug�������ClearType����������������������ض��ֺ������Ե����ַ�
//��ʾʱ�ͻ�ǳ����ԣ���˲��Ƽ�С�ֺ�����ʹ�á��Ƽ�ʹ��Direct2D+DirectWrite�Ű�ϵͳ�������ֵ�ʸ��ͼ�Ρ�
HRESULT LoadFontFromSystem(ID3D11Device* device, std::unique_ptr<DirectX::SpriteFont> &outSF, unsigned textureWidth,
	unsigned textureHeight, LPWSTR fontName, float fontSize, const D2D1_COLOR_F &fontColor,
	DWRITE_FONT_WEIGHT fontWeight, wchar_t *pszCharacters = NULL, float pxBetweenChar = 1.0f,
	bool convertpmalpha = true);
//�����С�ĵ�λΪͨ��������ֺ�
HRESULT DrawTextToTexture(ID3D11Device* device, LPWSTR text, ID3D11ShaderResourceView** pTex, int* pw, int* ph,
	LPWSTR fontName, float fontSize, const D2D1_COLOR_F& fontColor,
	DWRITE_FONT_WEIGHT fontWeight, bool convertpmalpha = true);
//������Ļͼ�����ļ�(PNG)
HRESULT TakeScreenShotToFile(ID3D11Device* device, IDXGISwapChain*swapChain,LPWSTR fpath);

//COM����������ͼ�����ļ�(PNG)
HRESULT SaveWicBitmapToFile(IWICBitmap *wicbitmap, LPCWSTR path);
//COM����������ͼ�����ڴ�(PNG)
HRESULT SaveWicBitmapToMemory(IWICBitmap *wicbitmap, std::unique_ptr<BYTE> &outMem, size_t *pbytes);
//COM���������ڴ��д�ͼ��
HRESULT LoadWicBitmapFromMemory(Microsoft::WRL::ComPtr<IWICBitmap> &outbitmap, BYTE *mem, size_t memsize);
int ReadFileToMemory(const char *pfilename, std::unique_ptr<char> &memout, size_t *memsize,
	bool removebom = false);
//��WICͼ��ת���ɣ�
//pmalpha=true:Ԥ��͸���ȸ�ʽ��PNG��
//pmalpha=false:ֱ��͸���ȸ�ʽ��PNG��
//��ע�⡿�������ɫ������ֵΪ0���޷�ת��Ϊֱ��͸���ȸ�ʽ��
HRESULT WicBitmapConvertPremultiplyAlpha(IWICBitmap *wicbitmap,
	Microsoft::WRL::ComPtr<IWICBitmap> &outbitmap, bool pmalpha);
//����������������D2D��������������������
//COM��������Ҫ����CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE),��Ҫ����� WindowsCodecs.lib.
//��ʾ��DXTK��ԭ����ͼ����ƹ��ܣ���ʹ��ResLoader::LoadTextureFromFile��ͼƬֱ�Ӽ���ΪID3D11ShaderResourceView���͵���Դ��
HRESULT CreateD2DImageFromFile(Microsoft::WRL::ComPtr<ID2D1Bitmap> &pic, ID2D1RenderTarget *prt, LPCWSTR ppath);
//����������������DWrite��������������������
//��Ҫ����� DWrite.lib.
HRESULT CreateDWTextFormat(Microsoft::WRL::ComPtr<IDWriteTextFormat> &textformat,
	LPCWSTR fontName, DWRITE_FONT_WEIGHT fontWeight,
	FLOAT fontSize, DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL,
	DWRITE_FONT_STRETCH fontExpand = DWRITE_FONT_STRETCH_NORMAL, LPCWSTR localeName = L"");
//��ע�⡿�������Ʊ��뱣֤����Ч�ġ�
HRESULT CreateDWFontFace(Microsoft::WRL::ComPtr<IDWriteFontFace> &fontface, LPCWSTR fontName,
	DWRITE_FONT_WEIGHT fontWeight, DWRITE_FONT_STYLE fontStyle, DWRITE_FONT_STRETCH fontExpand);
HRESULT CreateDWFontFace(Microsoft::WRL::ComPtr<IDWriteFontFace> &fontface, IDWriteTextFormat *textformat);
//����ָ��������������Ӧ������·������������������Լ��߼���ɫ��䣬���ʺ�Ƶ�����ã����ֻ������ȷ�����ı���
//factory����ָ��Ϊ����ȾĿ��ʹ����ͬ�Ķ���
//��ע�⡿�ú�����֧�ֻ��з���
HRESULT CreateD2DGeometryFromText(Microsoft::WRL::ComPtr<ID2D1PathGeometry> &geometry, ID2D1Factory *factory,
	IDWriteFontFace *pfontface, IDWriteTextFormat* textformat, const wchar_t *text, size_t textlength);
//����һ����һ�α仯�Ľ����ˢ��
//��ע�⡿�ú����е�����������ͼ�ε����½�Ϊԭ�㣬����ΪX������������ΪY��������
HRESULT CreateD2DLinearGradientBrush(Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> &lgBrush, ID2D1RenderTarget *rt,
	float startX, float startY, float endX, float endY, D2D1_COLOR_F startColor, D2D1_COLOR_F endColor);
//���ƴ���������·����������BeginDraw��EndDraw֮����á���ɺ���ȾĿ���λ�ý�����ԭ�㡣
void D2DDrawGeometryWithOutline(ID2D1RenderTarget *rt, ID2D1Geometry *geometry, float x, float y,
	ID2D1Brush *fillBrush, ID2D1Brush *outlineBrush, float outlineWidth);
//����һ��Բ����ʹ�ýǶ��ƣ�ˮƽ����Ϊ0�ȣ�˳ʱ��Ϊ������
//endDegree�������startDegree��factory����ָ��Ϊ����ȾĿ��ʹ����ͬ�Ķ���
HRESULT CreateD2DArc(Microsoft::WRL::ComPtr<ID2D1PathGeometry> &arc, ID2D1Factory *factory, float r,
	float startDegree, float endDegree);
//���ƷǷ�յ�·����������BeginDraw��EndDraw֮����á���ɺ���ȾĿ���λ�ý�����ԭ�㡣
void D2DDrawPath(ID2D1RenderTarget *rt, ID2D1PathGeometry *path, float x, float y, ID2D1Brush *color, float width);
