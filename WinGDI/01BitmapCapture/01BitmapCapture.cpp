#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windowsx.h>
#include <atlstr.h>
#include <algorithm>
#include <cassert>

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
	int winhcap, winedge;

	wchar_t const* ClassName() const { return L"MyWnd"; };
	LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp);

public:
	MyWnd();
	~MyWnd();

	HWND Get() const { return wnd; }

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

	BOOL Capture();
	BOOL Save();
	BOOL Save2();
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

	winhcap = GetSystemMetrics(SM_CYCAPTION);
	winedge = 16;
	CreateWindowExW(exstyle, ClassName(), title, style,
		x, y, width + winedge, height + winedge + winhcap,
		parent, menu, GetModuleHandleW(NULL), this);

	return wnd != NULL;
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


MyWnd::MyWnd()
{
	wnd = NULL;
}


MyWnd::~MyWnd()
{
	// if (wnd)
		// CloseWindow(wnd);
}


LRESULT MyWnd::HandleMessage(UINT msg, WPARAM wp, LPARAM lp)
{
	PAINTSTRUCT ps;
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
		BeginPaint(wnd, &ps);
		Capture();
		if (save)
			Save();
		EndPaint(wnd, &ps);
		return 0;
	}
	return DefWindowProcW(wnd, msg, wp, lp);
}


BOOL MyWnd::Capture()
{
	HDC dcScreen = NULL, dcWindow = NULL;
	RECT rc;
	BOOL ret = TRUE;
	int colScreen = GetSystemMetrics(SM_CXSCREEN);
	int rowScreen = GetSystemMetrics(SM_CYSCREEN);

	// 查找当前客户区域显示设备上下文的句柄
	dcScreen = GetDC(NULL);
	dcWindow = GetDC(wnd);
	GetClientRect(wnd, &rc);
	SetStretchBltMode(dcWindow, HALFTONE);

	// The source DC is the entire screen and the destination DC is the current window (HWND)
	ret = StretchBlt(
		dcWindow, 0, 0, rc.right, rc.bottom,
		dcScreen, 0, 0, colScreen, rowScreen,
		SRCCOPY);
	if (!ret)
		PopUpError(L"StretchBlt 失败，不能捕获截图");

	ReleaseDC(NULL, dcScreen);
	ReleaseDC(wnd, dcWindow);
	return ret;
}


BOOL MyWnd::Save2()
{
	HDC dcScreen, dcMem = NULL;
	HBITMAP objBmp = NULL;
	HGDIOBJ objOld = NULL;
	BITMAPFILEHEADER fileHdr = { 0 };
	BITMAPINFO infoBmp = { 0 };
	BITMAPINFOHEADER& infoHdr = infoBmp.bmiHeader;
	unsigned palette[256];
	BOOL ret = TRUE;
	int colScreen = GetSystemMetrics(SM_CXSCREEN);
	int rowScreen = GetSystemMetrics(SM_CYSCREEN);
	LPVOID dptr = NULL;
	dcScreen = GetDC(NULL);
	dcMem = CreateCompatibleDC(NULL);

	do
	{
		if (!dcMem)
		{
			PopUpError(L"CreateCompatibleDC 失败，不能创建中间上下文");
			break;
		}

		objBmp = CreateCompatibleBitmap(dcScreen, colScreen, rowScreen);
		if (!objBmp)
		{
			PopUpError(L"CreateCompatibleBitmap 失败");
		}

		objOld = SelectObject(dcMem, objBmp);
		DeleteObject(objOld);

		// Bit block transfer into our compatible memory DC
		ret = BitBlt(
			dcMem, 0, 0, colScreen, rowScreen,
			dcScreen, 0, 0, SRCCOPY);
		if (!ret)
		{
			PopUpError(L"BitBlt 失败，不能复制截图到当前位图");
			break;
		}

		infoBmp.bmiHeader.biSize = sizeof(infoHdr);
		ret = GetDIBits(dcScreen, objBmp, 0, colScreen,
			NULL, &infoBmp, DIB_RGB_COLORS);
		if (!ret)
		{
			PopUpError(L"不能获取当前位图信息");
			break;
		}

		int channel = infoHdr.biBitCount / 8;
		int step = (infoHdr.biWidth * channel + 3) & -4;
		int szImg = step * infoHdr.biHeight;
		int szHdr = sizeof(fileHdr) + sizeof(infoHdr);
		dptr = HeapAlloc(GetProcessHeap(), 0, szImg);
		fileHdr.bfType = 0x4D42;
		fileHdr.bfSize = szImg + szHdr;
		fileHdr.bfOffBits = szHdr;
		infoHdr.biCompression = BI_RGB;
		if (!dptr)
		{
			MessageBoxW(wnd, L"内存不足", L"错误", MB_OK);
			break;
		}

		ret = GetDIBits(dcScreen, objBmp, 0, infoHdr.biHeight,
			dptr, &infoBmp, DIB_RGB_COLORS);
		if (!ret)
		{
			PopUpError(L"不能获取位图数据");
			break;
		}

		SYSTEMTIME T;
		CStringW E;
		GetSystemTime(&T);
		E.Format(
			LR"~(捕获-%hu%02hu%02hu_%02hu%02hu%02hu_%03hu.bmp)~",
			T.wYear, T.wMonth, T.wDay,
			T.wHour, T.wMinute, T.wSecond, T.wMilliseconds);
		DWORD written = 0;
		HANDLE fid = CreateFileW(E.GetString(), GENERIC_WRITE,
			0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (!fid)
		{
			PopUpError(L"创建文件失败");
			break;
		}
		WriteFile(fid, &fileHdr, sizeof(fileHdr), &written, NULL);
		WriteFile(fid, &infoHdr, sizeof(infoHdr), &written, NULL);
		if (channel == 1)
		{
			for (int i = 0; i < 256; ++i)
				palette[i] = i + (i << 8) + (i << 16);
			WriteFile(fid, palette, sizeof(palette), &written, NULL);
		}
		WriteFile(fid, dptr, szImg, &written, NULL);
		CloseHandle(fid);
		// MessageBoxW(wnd, E.GetString(), L"保存文件", MB_OK);
	} while (0, 0);

	if (dptr)
		HeapFree(GetProcessHeap(), 0, dptr);
	if (objOld)
		SelectObject(dcMem, objOld);
	if (dcMem)
		DeleteDC(dcMem);
	if (objBmp)
		DeleteObject(objBmp);
	ReleaseDC(NULL, dcScreen);
	return ret;
}


BOOL MyWnd::Save()
{
	HDC dcScreen, dcMem = NULL;
	HBITMAP objBmp = NULL;
	HGDIOBJ objOld = NULL;
	BITMAP bmp;
	BITMAPFILEHEADER fileHdr = { 0 };
	BOOL ret = TRUE;
	int colScreen = GetSystemMetrics(SM_CXSCREEN);
	int rowScreen = GetSystemMetrics(SM_CYSCREEN);
	dcScreen = GetDC(NULL);
	dcMem = CreateCompatibleDC(NULL);

	do
	{
		if (!dcMem)
		{
			PopUpError(L"CreateCompatibleDC 失败，不能创建中间上下文");
			break;
		}

		UCHAR buffer[sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD)];
		BITMAPINFO* infoHdr = (BITMAPINFO*)(buffer);
		FillBitmapInfo(infoHdr, colScreen, rowScreen, 24, 1);
		objBmp = CreateDIBSection(dcMem, infoHdr, DIB_RGB_COLORS, NULL, 0, 0);
		if (!objBmp)
		{
			PopUpError(L"CreateCompatibleBitmap 失败");
			break;
		}

		// Bit block transfer into our compatible memory DC
		objOld = SelectObject(dcMem, objBmp);
		ret = BitBlt(
			dcMem, 0, 0, colScreen, rowScreen,
			dcScreen, 0, 0, SRCCOPY);
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
			LR"~(捕获-%hu%02hu%02hu_%02hu%02hu%02hu_%03hu.bmp)~",
			T.wYear, T.wMonth, T.wDay,
			T.wHour, T.wMinute, T.wSecond, T.wMilliseconds);
		SaveBitmap(E.GetString(), bmp);
	} while (0, 0);

	if (objOld)
		SelectObject(dcMem, objOld);
	if (dcMem)
		DeleteObject(dcMem);
	if (objBmp)
		DeleteObject(objBmp);
	DeleteObject(dcScreen);
	ReleaseDC(NULL, dcScreen);
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
		CW_USEDEFAULT, CW_USEDEFAULT, 800, 450)))
	{
		PopUpError(L"创建窗口失败");
		return -1;
	}

	ShowWindow(m.Get(), cmdshow);
	UpdateWindow(m.Get());

	// Run message loop
	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		else
		{
			Sleep(50);
			InvalidateRect(m.Get(), NULL, FALSE);
		}
	}

	return static_cast<int>(msg.wParam);
}