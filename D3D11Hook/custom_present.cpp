#include<d2d1.h>
#include<d3d11.h>
#include<dwrite.h>
#include<wincodec.h>
#include<wrl\client.h>
#include<vector>
#include<DirectXMath.h>
#include<memory>
#include<fstream>
#include<Shlwapi.h>
#include"custom_present.h"
#pragma comment(lib,"d2d1.lib")
#pragma comment(lib,"dwrite.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"windowscodecs.lib")

#define MAX_TEXTURE_DIMENSION 8192//参考：https://msdn.microsoft.com/library/windows/desktop/ff476876.aspx

#ifdef _DEBUG
HRESULT _debug_hr = S_OK;
#define C(sth) _debug_hr=sth;if(FAILED(_debug_hr))return _debug_hr
#else
#define C(sth) sth
#endif

using Microsoft::WRL::ComPtr;

namespace DX
{
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			// Set a breakpoint on this line to catch DirectX API errors
			throw hr;
		}
	}
}

//COM函数，保存图像至文件
HRESULT SaveWicBitmapToFile(IWICBitmap *wicbitmap, LPCWSTR path)
{
	//http://blog.csdn.net/augusdi/article/details/9051723
	IWICImagingFactory *wicfactory;
	C(CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory,
		(void**)&wicfactory));
	IWICStream *wicstream;
	C(wicfactory->CreateStream(&wicstream));
	C(wicstream->InitializeFromFilename(path, GENERIC_WRITE));
	IWICBitmapEncoder *benc;
	IWICBitmapFrameEncode *bfenc;
	C(wicfactory->CreateEncoder(GUID_ContainerFormatPng, NULL, &benc));
	C(benc->Initialize(wicstream, WICBitmapEncoderNoCache));
	C(benc->CreateNewFrame(&bfenc, NULL));
	C(bfenc->Initialize(NULL));
	UINT w, h;
	C(wicbitmap->GetSize(&w, &h));
	C(bfenc->SetSize(w, h));
	WICPixelFormatGUID format = GUID_WICPixelFormatDontCare;
	C(bfenc->SetPixelFormat(&format));
	C(bfenc->WriteSource(wicbitmap, NULL));
	C(bfenc->Commit());
	C(benc->Commit());
	benc->Release();
	bfenc->Release();
	wicstream->Release();
	wicfactory->Release();
	return S_OK;
}

//COM函数，保存图像至内存，需要手动释放(delete)
HRESULT SaveWicBitmapToMemory(IWICBitmap *wicbitmap, std::unique_ptr<BYTE> &outMem, size_t *pbytes)
{
	//http://blog.csdn.net/augusdi/article/details/9051723
	IWICImagingFactory *wicfactory;
	C(CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory,
		(void**)&wicfactory));
	IWICStream *wicstream;
	IStream *memstream = SHCreateMemStream(NULL, 0);
	if (!memstream)return -1;
	C(wicfactory->CreateStream(&wicstream));
	C(wicstream->InitializeFromIStream(memstream));
	IWICBitmapEncoder *benc;
	IWICBitmapFrameEncode *bfenc;
	C(wicfactory->CreateEncoder(GUID_ContainerFormatPng, NULL, &benc));
	C(benc->Initialize(wicstream, WICBitmapEncoderNoCache));
	C(benc->CreateNewFrame(&bfenc, NULL));
	C(bfenc->Initialize(NULL));
	UINT w, h;
	C(wicbitmap->GetSize(&w, &h));
	C(bfenc->SetSize(w, h));
	WICPixelFormatGUID format = GUID_WICPixelFormatDontCare;
	C(bfenc->SetPixelFormat(&format));
	C(bfenc->WriteSource(wicbitmap, NULL));
	C(bfenc->Commit());
	C(benc->Commit());

	ULARGE_INTEGER ulsize;
	C(wicstream->Seek({ 0,0 }, STREAM_SEEK_SET, NULL));
	C(IStream_Size(wicstream, &ulsize));
	*pbytes = (size_t)ulsize.QuadPart;
	outMem.reset(new BYTE[*pbytes]);
	ULONG readbytes;
	C(wicstream->Read(outMem.get(), (ULONG)ulsize.QuadPart, &readbytes));
	if (readbytes != ulsize.QuadPart)
		return -1;
	benc->Release();
	bfenc->Release();
	wicstream->Release();
	memstream->Release();
	wicfactory->Release();
	return S_OK;
}

HRESULT LoadWicBitmapFromMemory(ComPtr<IWICBitmap> &outbitmap, BYTE *mem, size_t memsize)
{
	IWICImagingFactory *wicfactory;
	C(CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory,
		(void**)&wicfactory));
	IStream *memstream = SHCreateMemStream(mem, (UINT)memsize);
	IWICBitmapDecoder *wicimgdecoder;
	C(wicfactory->CreateDecoderFromStream(memstream, NULL, WICDecodeMetadataCacheOnDemand, &wicimgdecoder));
	IWICFormatConverter *wicimgsource;
	C(wicfactory->CreateFormatConverter(&wicimgsource));
	IWICBitmapFrameDecode *picframe;
	C(wicimgdecoder->GetFrame(0, &picframe));
	C(wicimgsource->Initialize(picframe, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.0f,
		WICBitmapPaletteTypeCustom));
	C(wicfactory->CreateBitmapFromSource(wicimgsource, WICBitmapNoCache, outbitmap.ReleaseAndGetAddressOf()));
	C(picframe->Release());
	C(wicimgsource->Release());
	C(wicimgdecoder->Release());
	C(memstream->Release());
	C(wicfactory->Release());
	return S_OK;
}

int ReadFileToMemory(const char *pfilename, std::unique_ptr<char>& memout, size_t *memsize, bool removebom)
{
	std::ifstream fin(pfilename, std::ios::in | std::ios::binary);
	if (!fin)return -1;
	int startpos = 0;
	if (removebom)
	{
		char bom[3] = "";
		fin.read(bom, sizeof bom);
		if (strncmp(bom, "\xff\xfe", 2) == 0)startpos = 2;
		if (strncmp(bom, "\xfe\xff", 2) == 0)startpos = 2;
		if (strncmp(bom, "\xef\xbb\xbf", 3) == 0)startpos = 3;
	}
	fin.seekg(0, std::ios::end);
	*memsize = (int)fin.tellg() - startpos;
	memout.reset(new char[*memsize]);
	fin.seekg(startpos, std::ios::beg);
	fin.read(memout.get(), *memsize);
	return 0;
}

//COM函数
HRESULT CreateD2DImageFromFile(ComPtr<ID2D1Bitmap> &pic, ID2D1RenderTarget *prt, LPCWSTR ppath)
{
	IWICImagingFactory *wicfactory;
	C(CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory,
		(void**)&wicfactory));
	IWICBitmapDecoder *wicimgdecoder;
	C(wicfactory->CreateDecoderFromFilename(ppath, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &wicimgdecoder));
	IWICFormatConverter *wicimgsource;
	C(wicfactory->CreateFormatConverter(&wicimgsource));
	IWICBitmapFrameDecode *picframe;
	C(wicimgdecoder->GetFrame(0, &picframe));
	C(wicimgsource->Initialize(picframe, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.0f,
		WICBitmapPaletteTypeCustom));
	if (FAILED(prt->CreateBitmapFromWicBitmap(wicimgsource, NULL, pic.ReleaseAndGetAddressOf())))
		return -2;
	C(picframe->Release());
	C(wicimgsource->Release());
	C(wicimgdecoder->Release());
	C(wicfactory->Release());
	return S_OK;
}

HRESULT CreateDWTextFormat(ComPtr<IDWriteTextFormat> &textformat,
	LPCWSTR fontName, DWRITE_FONT_WEIGHT fontWeight,
	FLOAT fontSize, DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL,
	DWRITE_FONT_STRETCH fontExpand = DWRITE_FONT_STRETCH_NORMAL, LPCWSTR localeName = L"")
{
	ComPtr<IDWriteFactory> dwfactory;
	C(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(dwfactory), (IUnknown**)&dwfactory));
	C(dwfactory->CreateTextFormat(fontName, NULL, fontWeight, fontStyle, fontExpand, fontSize, localeName,
		textformat.ReleaseAndGetAddressOf()));
	return S_OK;
}

HRESULT CreateDWFontFace(ComPtr<IDWriteFontFace>& fontface, LPCWSTR fontName,
	DWRITE_FONT_WEIGHT fontWeight, DWRITE_FONT_STYLE fontStyle, DWRITE_FONT_STRETCH fontExpand)
{
	ComPtr<IDWriteFactory> dwfactory;
	C(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(dwfactory), (IUnknown**)&dwfactory));
	ComPtr<IDWriteFontCollection> fontcollection;
	C(dwfactory->GetSystemFontCollection(&fontcollection));
	UINT fontfamilypos;
	BOOL ffexists;
	C(fontcollection->FindFamilyName(fontName, &fontfamilypos, &ffexists));
	ComPtr<IDWriteFontFamily> ffamily;
	C(fontcollection->GetFontFamily(fontfamilypos, &ffamily));
	ComPtr<IDWriteFontList> flist;
	C(ffamily->GetMatchingFonts(fontWeight, fontExpand, fontStyle, &flist));
	ComPtr<IDWriteFont> dwfont;
	C(flist->GetFont(0, &dwfont));
	C(dwfont->CreateFontFace(&fontface));
	return S_OK;
}

HRESULT CreateDWFontFace(ComPtr<IDWriteFontFace>& fontface, IDWriteTextFormat * textformat)
{
	TCHAR fontName[256];
	C(textformat->GetFontFamilyName(fontName, ARRAYSIZE(fontName)));
	C(CreateDWFontFace(fontface, fontName,
		textformat->GetFontWeight(), textformat->GetFontStyle(), textformat->GetFontStretch()));
	return S_OK;
}

constexpr float PointToDip(float pointsize)
{
	//https://www.codeproject.com/articles/376597/outline-text-with-directwrite#source
	return pointsize * 96.0f / 72.0f;
}

HRESULT CreateD2DGeometryFromText(ComPtr<ID2D1PathGeometry>& geometry, ID2D1Factory *factory,
	IDWriteFontFace* pfontface, float fontSize, const wchar_t * text, size_t textlength)
{
	std::vector<UINT32> unicode_ui32;
	for (size_t i = 0; i < textlength; i++)
		unicode_ui32.push_back(static_cast<UINT32>(text[i]));
	std::unique_ptr<UINT16>gidcs(new UINT16[textlength]);
	C(pfontface->GetGlyphIndicesW(unicode_ui32.data(), static_cast<UINT32>(unicode_ui32.size()), gidcs.get()));
	C(factory->CreatePathGeometry(geometry.ReleaseAndGetAddressOf()));
	ID2D1GeometrySink *gs;
	C(geometry->Open(&gs));
	C(pfontface->GetGlyphRunOutline(PointToDip(fontSize), gidcs.get(), NULL, NULL,
		static_cast<UINT32>(textlength), FALSE, FALSE, gs));
	C(gs->Close());
	gs->Release();
	return S_OK;
}

HRESULT CreateD2DLinearGradientBrush(ComPtr<ID2D1LinearGradientBrush>& lgBrush, ID2D1RenderTarget * rt,
	float startX, float startY, float endX, float endY, D2D1_COLOR_F startColor, D2D1_COLOR_F endColor)
{
	//https://msdn.microsoft.com/zh-cn/library/dd756678(v=vs.85).aspx
	ComPtr<ID2D1GradientStopCollection> gsc;
	D2D1_GRADIENT_STOP gs[2] = { {0.0f,startColor},{1.0f,endColor} };
	C(rt->CreateGradientStopCollection(gs, ARRAYSIZE(gs), gsc.ReleaseAndGetAddressOf()));
	C(rt->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(startX, startY),
		D2D1::Point2F(endX, endY)), gsc.Get(), lgBrush.ReleaseAndGetAddressOf()));
	return S_OK;
}

void D2DDrawGeometryWithOutline(ID2D1RenderTarget * rt, ID2D1Geometry * geometry,
	float x, float y, ID2D1Brush * fillBrush, ID2D1Brush * outlineBrush, float outlineWidth)
{
	rt->SetTransform(D2D1::Matrix3x2F::Translation(x, y));
	rt->FillGeometry(geometry, fillBrush);
	rt->DrawGeometry(geometry, outlineBrush, outlineWidth);
	rt->SetTransform(D2D1::IdentityMatrix());
}

HRESULT CreateD2DArc(ComPtr<ID2D1PathGeometry> &arc, ID2D1Factory *factory, float r, float startDegree, float endDegree)
{
	ComPtr<ID2D1GeometrySink> sink;
	C(factory->CreatePathGeometry(arc.ReleaseAndGetAddressOf()));
	C(arc->Open(sink.ReleaseAndGetAddressOf()));
	sink->BeginFigure(D2D1::Point2F(r*cosf(DirectX::XMConvertToRadians(startDegree)),
		r*sinf(DirectX::XMConvertToRadians(startDegree))), D2D1_FIGURE_BEGIN_HOLLOW);
	sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(r*cosf(DirectX::XMConvertToRadians(endDegree)),
		r*sinf(DirectX::XMConvertToRadians(endDegree))), D2D1::SizeF(r, r), 0, D2D1_SWEEP_DIRECTION_CLOCKWISE,
		endDegree - startDegree > 180.0f ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL));
	sink->EndFigure(D2D1_FIGURE_END_OPEN);
	C(sink->Close());
	return S_OK;
}

void D2DDrawPath(ID2D1RenderTarget *rt, ID2D1PathGeometry *path, float x, float y, ID2D1Brush *color, float width)
{
	rt->SetTransform(D2D1::Matrix3x2F::Translation(x, y));
	rt->DrawGeometry(path, color, width);
	rt->SetTransform(D2D1::IdentityMatrix());
}

class D2DCustomPresent
{
private:
	ComPtr<ID2D1Factory> d2ddevice;
	ComPtr<ID2D1RenderTarget> d2drendertarget;//D2D绘图目标
	ComPtr<IDWriteFontFace> fontface;//DWrite字体Face
	ComPtr<IDWriteTextFormat> textformat;//DWrite排版系统的字体
	ComPtr<ID2D1LinearGradientBrush> lgBrush;//渐变色笔刷
	ComPtr<ID2D1SolidColorBrush> whiteBrush;//纯色笔刷
	DXGI_SURFACE_DESC dxgisurface;
	TCHAR odText[10];//显示信息的文字
public:
	bool needinit;
	D2DCustomPresent():needinit(true)
	{
	}
	BOOL Init(IDXGISwapChain*pSC)
	{
		DX::ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2ddevice.ReleaseAndGetAddressOf()));
		IDXGISurface *surface;
		pSC->GetBuffer(0, IID_PPV_ARGS(&surface));
		surface->GetDesc(&dxgisurface);
		DX::ThrowIfFailed(d2ddevice->CreateDxgiSurfaceRenderTarget(surface, D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)), d2drendertarget.ReleaseAndGetAddressOf()));
		surface->Release();

		d2drendertarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), whiteBrush.ReleaseAndGetAddressOf());
		CreateD2DLinearGradientBrush(lgBrush, d2drendertarget.Get(), 0, -50, 0, 0,
			D2D1::ColorF(D2D1::ColorF::Blue), D2D1::ColorF(D2D1::ColorF::White));
		CreateDWTextFormat(textformat, L"宋体", DWRITE_FONT_WEIGHT_NORMAL, 48.0f);
		CreateDWFontFace(fontface, textformat.Get());
		textformat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		//textformat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		lstrcpy(odText, TEXT("砸瓦路多！！"));
		return TRUE;
	}
	void Draw()
	{
		//D2D绘图方法参考：https://github.com/lxfly2000/dxtkwithd2d/blob/master/Game/GameLite.cpp
		d2drendertarget->BeginDraw();
		//D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT：绘制彩色文字，要求系统为Win8.1/10，在Win7中会使D2D区域无法显示
		//D2D1_DRAW_TEXT_OPTIONS_NONE：不使用高级绘制选项
		d2drendertarget->DrawText(odText, lstrlen(odText), textformat.Get(),
			D2D1::RectF(0.0f, 0.0f, (float)dxgisurface.Width,(float)dxgisurface.Height), whiteBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);
		d2drendertarget->EndDraw();
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