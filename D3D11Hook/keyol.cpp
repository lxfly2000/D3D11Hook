#include"keyol.h"
#include"custom_present.h"
#include<string>
#include<vector>
#include<sstream>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

char* UnicodeToUtf8(const wchar_t* unicode,char *szUtf8,size_t cchUtf8)
{
	int len;
	len = WideCharToMultiByte(CP_UTF8, 0, unicode, -1, NULL, 0, NULL, NULL);
	if (cchUtf8 < len + 1)
		return nullptr;
	memset(szUtf8, 0, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, unicode, -1, szUtf8, len, NULL, NULL);
	return szUtf8;
}

LRESULT CALLBACK KeyOverlayExtraProcess(WNDPROC oldProc, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		return TRUE;
	return CallWindowProc(oldProc, hwnd, msg, wParam, lParam);
}

static ImVec4 textColor1, textColor2, textColor3, bgColor1, bgColor2, bgColor3;
char ui_title_utf8[32];
struct KeyEntry
{
	int vkCode;
	char name[8];
	float width;
	float height;
	bool next_line;
};
std::vector<KeyEntry>keys;

BOOL KeyOverlayInit(HWND hwnd, ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	TCHAR szConfPath[MAX_PATH];
	GetDLLPath(szConfPath, ARRAYSIZE(szConfPath));
	lstrcpy(wcsrchr(szConfPath, '.'), TEXT(".ini"));
#define GetInitConfStr(key,def) GetPrivateProfileString(TEXT("KeyOverlay"),TEXT(_STRINGIZE(key)),def,key,ARRAYSIZE(key),szConfPath)
#define GetInitConfInt(key,def) key=GetPrivateProfileInt(TEXT("KeyOverlay"),TEXT(_STRINGIZE(key)),def,szConfPath)
#define F(_i_str) (float)_wtof(_i_str)
	size_t rlen;
	char font_path[MAX_PATH];
	getenv_s(&rlen, font_path, "windir");
	strcat_s(font_path, "/Fonts/SimSun.ttc");

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = NULL;
	int ui_style;
	GetInitConfInt(ui_style, 0);
	TCHAR ui_title[32];
	GetInitConfStr(ui_title, TEXT("KeyOverlay"));
	if (!UnicodeToUtf8(ui_title, ui_title_utf8, ARRAYSIZE(ui_title_utf8)))
		return FALSE;
	switch (ui_style)
	{
	case 0:default:ImGui::StyleColorsClassic(); break;
	case 1:ImGui::StyleColorsDark(); break;
	case 2:ImGui::StyleColorsLight(); break;
	}
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(pDevice, pContext);
	TCHAR font_size[16];
	GetInitConfStr(font_size, TEXT("18"));
	ImFontGlyphRangesBuilder fgb;
	static const ImWchar16 char_range[] = { 0x20,0xFF,0x2190,0x2193,0};
	ImFont* font = io.Fonts->AddFontFromFileTTF(font_path, F(font_size) * USER_DEFAULT_SCREEN_DPI/72,NULL,char_range);
	if (!font)
		return FALSE;
	TCHAR textColor1_red[16], textColor1_green[16], textColor1_blue[16], textColor1_alpha[16];
	TCHAR textColor2_red[16], textColor2_green[16], textColor2_blue[16], textColor2_alpha[16];
	TCHAR textColor3_red[16], textColor3_green[16], textColor3_blue[16], textColor3_alpha[16];
	TCHAR bgColor1_red[16], bgColor1_green[16], bgColor1_blue[16], bgColor1_alpha[16];
	TCHAR bgColor2_red[16], bgColor2_green[16], bgColor2_blue[16], bgColor2_alpha[16];
	TCHAR bgColor3_red[16], bgColor3_green[16], bgColor3_blue[16], bgColor3_alpha[16];
	GetInitConfStr(textColor1_red, TEXT("1"));
	GetInitConfStr(textColor1_green, TEXT("1"));
	GetInitConfStr(textColor1_blue, TEXT("1"));
	GetInitConfStr(textColor1_alpha, TEXT("1"));
	GetInitConfStr(textColor2_red, TEXT("0"));
	GetInitConfStr(textColor2_green, TEXT("0"));
	GetInitConfStr(textColor2_blue, TEXT("0"));
	GetInitConfStr(textColor2_alpha, TEXT("1"));
	GetInitConfStr(textColor3_red, TEXT("1"));
	GetInitConfStr(textColor3_green, TEXT("1"));
	GetInitConfStr(textColor3_blue, TEXT("1"));
	GetInitConfStr(textColor3_alpha, TEXT("0.5"));
	GetInitConfStr(bgColor1_red, TEXT("0"));
	GetInitConfStr(bgColor1_green, TEXT("0"));
	GetInitConfStr(bgColor1_blue, TEXT("0"));
	GetInitConfStr(bgColor1_alpha, TEXT("1"));
	GetInitConfStr(bgColor2_red, TEXT("1"));
	GetInitConfStr(bgColor2_green, TEXT("1"));
	GetInitConfStr(bgColor2_blue, TEXT("1"));
	GetInitConfStr(bgColor2_alpha, TEXT("1"));
	GetInitConfStr(bgColor3_red, TEXT("0"));
	GetInitConfStr(bgColor3_green, TEXT("0"));
	GetInitConfStr(bgColor3_blue, TEXT("0"));
	GetInitConfStr(bgColor3_alpha, TEXT("0"));
	textColor1 = ImVec4(F(textColor1_red), F(textColor1_green), F(textColor1_blue), F(textColor1_alpha));
	textColor2 = ImVec4(F(textColor2_red), F(textColor2_green), F(textColor2_blue), F(textColor2_alpha));
	textColor3 = ImVec4(F(textColor3_red), F(textColor3_green), F(textColor3_blue), F(textColor3_alpha));
	bgColor1 = ImVec4(F(bgColor1_red), F(bgColor1_green), F(bgColor1_blue), F(bgColor1_alpha));
	bgColor2 = ImVec4(F(bgColor2_red), F(bgColor2_green), F(bgColor2_blue), F(bgColor2_alpha));
	bgColor3 = ImVec4(F(bgColor3_red), F(bgColor3_green), F(bgColor3_blue), F(bgColor3_alpha));

	keys.clear();
	TCHAR keykey[16],strbuf[128];
	for (int i = 0; i < 256; i++)
	{
		wsprintf(keykey, TEXT("key%d"), i);
		GetPrivateProfileString(TEXT("KeyOverlay"), keykey, TEXT(""), strbuf, ARRAYSIZE(strbuf), szConfPath);
		if (wcslen(strbuf) == 0)
			continue;
		std::wistringstream iss(strbuf);
		KeyEntry ke;
		ZeroMemory(&ke, sizeof ke);
		TCHAR kname[12];
		iss >> ke.vkCode >> kname >> ke.width >> ke.height >> ke.next_line;
		size_t offset = 0, clen = lstrlen(kname);
		if (kname[0] == '"' && kname[clen - 1] == '"')
		{
			for (size_t j = 0; j < clen - 2; j++)
				kname[j] = kname[j + 1];
			kname[clen - 2] = 0;
		}
		if (!UnicodeToUtf8(kname, ke.name, ARRAYSIZE(ke.name)))
			return FALSE;
		keys.push_back(ke);
	}
	return TRUE;
}

void KeyOverlayUninit()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void KeyOverlayDraw()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin(ui_title_utf8);

	for (size_t i = 0; i < keys.size(); i++)
	{
		const KeyEntry& ke = keys[i];
		if (ke.vkCode == 0)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, bgColor3);
			ImGui::PushStyleColor(ImGuiCol_Text, textColor3);
		}
		else if (GetAsyncKeyState(ke.vkCode) & 0x8000)//Pressed
		{
			ImGui::PushStyleColor(ImGuiCol_Button, bgColor2);
			ImGui::PushStyleColor(ImGuiCol_Text, textColor2);
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Button, bgColor1);
			ImGui::PushStyleColor(ImGuiCol_Text, textColor1);
		}
		ImGui::Button(ke.name, ImVec2(ke.width, ke.height));
		ImGui::PopStyleColor(2);
		if (!ke.next_line)
			ImGui::SameLine();
	}

	ImGui::End();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

static_assert(sizeof(SHORT) == 2,"Size of SHORT not equals 2.");
