#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windowsx.h>
#include <atlstr.h>
#include <algorithm>
#include <cassert>

__pragma(comment(lib, "Msimg32.lib"))

static void PopUpError(wchar_t const* info)
{
	wchar_t* msg = NULL;
	DWORD err = GetLastError();
	DWORD len = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, err, 0 /*MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)*/,
		reinterpret_cast<wchar_t*>(&msg), 0, NULL);
	if (len)
	{
		CStringW e(info);
		e.Append(L"\n");
		e.Append(msg);
		LocalFree(msg);
		MessageBoxW(NULL, e.GetString(), L"错误", MB_OK);
	}
	else
		MessageBoxW(NULL, L"未知错误", L"错误", MB_OK);
}


// frome OpenCV window_w32.cpp
static void FillBitmapInfo(BITMAPINFO* bmi, int width, int height, int bpp, int origin)
{
	assert(bmi && width >= 0 && height >= 0 && (bpp == 8 || bpp == 24 || bpp == 32));

	BITMAPINFOHEADER* bmih = &(bmi->bmiHeader);

	memset(bmih, 0, sizeof(*bmih));
	bmih->biSize = sizeof(BITMAPINFOHEADER);
	bmih->biWidth = width;
	bmih->biHeight = origin ? abs(height) : -abs(height);
	bmih->biPlanes = 1;
	bmih->biBitCount = (unsigned short)bpp;
	bmih->biCompression = BI_RGB;

	if (bpp == 8)
	{
		RGBQUAD* palette = bmi->bmiColors;
		for (int i = 0; i < 256; ++i)
		{
			palette[i].rgbBlue = palette[i].rgbGreen = palette[i].rgbRed = (BYTE)i;
			palette[i].rgbReserved = 0;
		}
	}
}


static BOOL SaveBitmap(wchar_t const* name, BITMAP const& src)
{
	BITMAPFILEHEADER file = { 0 };
	BITMAPINFOHEADER info = { 0 };
	int szImg = src.bmWidthBytes * src.bmHeight;
	int szHdr = sizeof(file) + sizeof(info);
	file.bfType = 0x4D42;
	file.bfSize = szImg + szHdr;
	file.bfOffBits = szHdr;
	info.biSize = sizeof(info);
	info.biWidth = src.bmWidth;
	info.biHeight = src.bmHeight;
	info.biPlanes = 1;
	info.biBitCount = src.bmBitsPixel;
	info.biCompression = BI_RGB;
	info.biSizeImage = szImg;

	DWORD written = 0;
	HANDLE fid = CreateFileW(name, GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!fid)
		return FALSE;
	WriteFile(fid, &file, sizeof(file), &written, NULL);
	WriteFile(fid, &info, sizeof(info), &written, NULL);
	WriteFile(fid, src.bmBits, szImg, &written, NULL);
	CloseHandle(fid);
	return TRUE;
}


////////////////////////////////////////////////////////////


class MyWnd
{
	static LRESULT CALLBACK WinProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

protected:
	HWND wnd;
	RECT edge;

	wchar_t const* ClassName() const { return L"MyWnd"; };
	LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp);

public:
	BOOL Create(
		wchar_t const* title,
		DWORD style, DWORD exstyle = 0,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		int width = 600,
		int height = 600,
		HWND parent = NULL,
		HMENU menu = NULL
	);
	BOOL Draw(HDC dcwnd);
	BOOL Save(HDC dcwnd);

	MyWnd::MyWnd() { wnd = NULL; }
	MyWnd::~MyWnd() {}
	HWND Get() const { return wnd; }
};


BOOL MyWnd::Create(
	wchar_t const* title, DWORD style, DWORD exstyle,
	int x, int y, int width, int height,
	HWND parent, HMENU menu)
{
	WNDCLASSW C = { 0 };
	C.style = CS_HREDRAW | CS_VREDRAW;
	C.lpfnWndProc = MyWnd::WinProc;
	C.hInstance = GetModuleHandleW(NULL);
	C.hCursor = LoadCursorW(NULL, IDC_HAND);
	C.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOWFRAME);
	C.lpszMenuName = NULL;
	C.lpszClassName = ClassName();
	RegisterClassW(&C);

	edge = { 0, 0, 0, 0 };
	AdjustWindowRectEx(&edge, style, !!menu, exstyle);
	CreateWindowExW(exstyle, ClassName(), title, style,
		x, y, width + edge.right - edge.left, height + edge.bottom - edge.top,
		parent, menu, GetModuleHandleW(NULL), this);
	return wnd ? TRUE : FALSE;
}


LRESULT MyWnd::WinProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	MyWnd* m = NULL;

	if (msg == WM_NCCREATE)
	{
		CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
		m = reinterpret_cast<MyWnd*>(cs->lpCreateParams);
		SetWindowLongPtrW(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(m));
		m->wnd = wnd;
	}
	else
		m = reinterpret_cast<MyWnd*>(GetWindowLongPtrW(wnd, GWLP_USERDATA));

	return m
		? m->HandleMessage(msg, wp, lp)
		: DefWindowProcW(wnd, msg, wp, lp);
}


////////////////////////////////////////////////////////////


LRESULT MyWnd::HandleMessage(UINT msg, WPARAM wp, LPARAM lp)
{
	PAINTSTRUCT ps;
	HDC hdc = NULL;
	bool save = false;
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_LBUTTONDOWN:
		save = true;
	case WM_MOVE:
	case WM_PAINT:
		hdc = BeginPaint(wnd, &ps);
		Draw(hdc);
		if (save)
			Save(hdc);
		EndPaint(wnd, &ps);
		return 0;
	}
	return DefWindowProcW(wnd, msg, wp, lp);
}


BOOL MyWnd::Draw(HDC dcwnd)
{
	HDC dc = NULL;
	HBITMAP objBmp = NULL;
	HGDIOBJ objOld = NULL;
	BITMAP src;
	POINT szwin, szbmp, szfive;
	RECT rc;
	BOOL ret = TRUE;
	BLENDFUNCTION F;

	do
	{
		GetClientRect(wnd, &rc);
		szwin = { rc.right - rc.left, rc.bottom - rc.top };
		if (szwin.x < 10 || szwin.y < 10)
			break;
		szwin.y /= 3;
		szfive.x = szwin.x / 5;
		szfive.y = szwin.y / 5;
		szbmp.x = szwin.x - szfive.x * 2;
		szbmp.y = szwin.y - szfive.y * 2;

		dc = CreateCompatibleDC(dcwnd);
		if (!dc)
		{
			PopUpError(L"CreateCompatibleDC 失败，不能创建中间上下文");
			break;
		}

		BITMAPINFO infoHdr = { 0 };
		FillBitmapInfo(&infoHdr, szbmp.x, szbmp.y, 32, 1);
		objBmp = CreateDIBSection(dc, &infoHdr, DIB_RGB_COLORS, NULL, 0, 0);
		if (!objBmp)
		{
			PopUpError(L"CreateCompatibleBitmap 失败");
			break;
		}

		objOld = SelectObject(dc, objBmp);
		ret = GetObjectW(objBmp, sizeof(src), &src);
		if (!ret)
		{
			PopUpError(L"不能获取当前位图信息");
			break;
		}

		// in top window area, constant alpha = 50%, but no source alpha 
		// the color format for each pixel is 0xaarrggbb  
		// set all pixels to blue and set source alpha to zero 
		for (int h = 0; h < szbmp.y; ++h)
		{
			UINT32* B = reinterpret_cast<UINT*>(src.bmBits) + szbmp.x * h;
			for (int w = 0; w < szbmp.x; ++w)
				B[w] = 0x000000ff;
		}
		F.BlendOp = AC_SRC_OVER;
		F.BlendFlags = 0;
		F.SourceConstantAlpha = 0x7f; // half of 0xff = 50% transparency
		F.AlphaFormat = 0; // ignore source alpha channel
		ret = AlphaBlend(
			dcwnd, szfive.x, szfive.y + 0 * szwin.y, szbmp.x, szbmp.y,
			dc, 0, 0, szbmp.x, szbmp.y, F);
		if (!ret)
		{
			PopUpError(L"AlphaBlend 失败");
			break;
		}

		// in middle window area, constant alpha = 100% (disabled), source  
		// alpha is 0 in middle of bitmap and opaque in rest of bitmap 
		for (int h = 0; h < szbmp.y; ++h)
		{
			UINT* B = reinterpret_cast<UINT*>(src.bmBits) + szbmp.x * h;
			for (int w = 0; w < szbmp.x; ++w)
			{
				// in middle of bitmap: source alpha = 0 (transparent). 
				// This means multiply each color component by 0x00. 
				// Thus, after AlphaBlend, we have a, 0x00 * r,  
				// 0x00 * g,and 0x00 * b (which is 0x00000000) 
				// for now, set all pixels to red 
				if (szfive.y < h && h < szbmp.y - szfive.y
					&& szfive.x < w && w < szbmp.x - szfive.x)
					B[w] = 0x7fff0000; // 0x00ff0000;
				else
					B[w] = 0xff0000ff;
			}
		}
		F.BlendOp = AC_SRC_OVER;
		F.BlendFlags = 0;
		F.SourceConstantAlpha = 0xff; // opaque (disable constant alpha) 
		F.AlphaFormat = AC_SRC_ALPHA;  // use source alpha
		ret = AlphaBlend(
			dcwnd, szfive.x, szfive.y + 1 * szwin.y, szbmp.x, szbmp.y,
			dc, 0, 0, szbmp.x, szbmp.y, F);
		if (!ret)
		{
			PopUpError(L"AlphaBlend 失败");
			break;
		}

		// bottom window area, use constant alpha = 75% and a changing 
		// source alpha. Create a gradient effect using source alpha, and  
		// then fade it even more with constant alpha 
		for (int h = 0; h < szbmp.y; ++h)
		{
			UINT* B = reinterpret_cast<UINT*>(src.bmBits) + szbmp.x * h;
			for (int w = 0; w < szbmp.x; ++w)
			{
				// for a simple gradient, base the alpha value on the x
				byte a = static_cast<byte>(w * 255.F / szbmp.x);
				UINT m = a * 256 / 255;
				B[w] = (a << 24)              // A
					+ (((0x00 * m) >> 8) << 16) // R
					+ (((0x00 * m) >> 8) << 8)  // G
					+ (((0xff * m) >> 8) << 0); // B
			}
		}
		F.BlendOp = AC_SRC_OVER;
		F.BlendFlags = 0;
		F.SourceConstantAlpha = 0xbf; // use constant alpha, 75% opaqueness 
		F.AlphaFormat = AC_SRC_ALPHA;  // use source alpha
		ret = AlphaBlend(
			dcwnd, szfive.x, szfive.y + 2 * szwin.y, szbmp.x, szbmp.y,
			dc, 0, 0, szbmp.x, szbmp.y, F);
		if (!ret)
		{
			PopUpError(L"AlphaBlend 失败");
			break;
		}
	} while (0, 0);

	if (objOld)
		SelectObject(dc, objOld);
	if (dc)
		DeleteDC(dc);
	if (objBmp)
		DeleteObject(objBmp);
	return ret;
}


BOOL MyWnd::Save(HDC dcwnd)
{
	HDC dc = NULL;
	HBITMAP objBmp = NULL;
	HGDIOBJ objOld = NULL;
	BITMAP bmp;
	BITMAPFILEHEADER fileHdr = { 0 };
	BOOL ret = TRUE;

	do
	{
		dc = CreateCompatibleDC(dcwnd);
		if (!dc)
		{
			PopUpError(L"CreateCompatibleDC 失败，不能创建中间上下文");
			break;
		}

		RECT rc;
		GetClientRect(wnd, &rc);
		POINT sz = { rc.right - rc.left, rc.bottom - rc.top };
		BITMAPINFO infoHdr = { 0 };
		FillBitmapInfo(&infoHdr, sz.x, sz.y, 24, 1);
		objBmp = CreateDIBSection(dc, &infoHdr, DIB_RGB_COLORS, NULL, 0, 0);
		if (!objBmp)
		{
			PopUpError(L"CreateCompatibleBitmap 失败");
			break;
		}

		// Bit block transfer into our compatible memory DC
		objOld = SelectObject(dc, objBmp);
		ret = BitBlt(dc, 0, 0, sz.x, sz.y, dcwnd, 0, 0, SRCCOPY);
		if (!ret)
		{
			PopUpError(L"BitBlt 失败，不能复制截图到当前位图");
			break;
		}
		ret = GetObjectW(objBmp, sizeof(bmp), &bmp);
		if (!ret)
		{
			PopUpError(L"不能获取当前位图信息");
			break;
		}

		SYSTEMTIME T;
		CStringW E;
		GetSystemTime(&T);
		E.Format(
			LR"~(%hu%02hu%02hu_%02hu%02hu%02hu_%03hu.bmp)~",
			T.wYear, T.wMonth, T.wDay,
			T.wHour, T.wMinute, T.wSecond, T.wMilliseconds);
		SaveBitmap(E.GetString(), bmp);
	} while (0, 0);

	if (objOld)
		SelectObject(dc, objOld);
	if (dc)
		DeleteDC(dc);
	if (objBmp)
		DeleteObject(objBmp);
	return ret;
}


////////////////////////////////////////////////////////////


int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR cmdline, int cmdshow)
{
	UNREFERENCED_PARAMETER(instance);
	UNREFERENCED_PARAMETER(cmdline);

	MyWnd m;
	if (!(m.Create(
		L"Bitmap Capture - 你好 ཁམས་བཟང་། こんにちは 안녕하세요",
		WS_OVERLAPPEDWINDOW, 0,
		CW_USEDEFAULT, CW_USEDEFAULT, 600, 600)))
	{
		PopUpError(L"创建窗口失败");
		return -1;
	}

	ShowWindow(m.Get(), cmdshow);
	UpdateWindow(m.Get());

	MSG msg = { 0 };
	while (GetMessageW(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return static_cast<int>(msg.wParam);
}