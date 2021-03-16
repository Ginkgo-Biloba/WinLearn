#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windowsx.h>
#include <atlstr.h>
#include <algorithm>
#include <cassert>
#include "resource.h"


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


/// OpenCV window_w32.cpp
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
	RECT edge, target;
	POINT P;
	int shape;

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
};


BOOL MyWnd::Create(
	wchar_t const* title, DWORD style, DWORD exstyle,
	int x, int y, int width, int height,
	HWND parent, HMENU menu)
{
	WNDCLASSW C = { 0 };

	C.style = CS_OWNDC;
	C.lpfnWndProc = MyWnd::WinProc;
	C.hInstance = GetModuleHandleW(NULL);
	C.hCursor = LoadCursorW(NULL, IDC_ARROW);
	C.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	C.lpszMenuName = NULL;
	C.lpszClassName = ClassName();
	C.lpszMenuName = MAKEINTRESOURCEW(IDR_MENU1);
	RegisterClassW(&C);
	edge = { 0, 0, 0, 0 };
	AdjustWindowRectEx(&edge, style, !!menu, exstyle);
	CreateWindowExW(
		exstyle, ClassName(), title, style, x, y,
		edge.right - edge.left + width,
		edge.bottom - edge.top + height,
		parent, menu, C.hInstance, this);

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
	HDC hdc = GetDC(wnd);
	HMENU menu = GetMenu(wnd);
	RECT C;
	int oldsh;
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_CREATE:
		shape = ID_FILLEDSHAPE_ELLIPSE;
		C.left = C.right = C.bottom = C.top = 0;
		target = C;
		CheckMenuItem(menu, shape, MF_CHECKED);
		return 0;

	case WM_COMMAND:
		oldsh = shape;
		shape = (int)(wp);
		CheckMenuItem(menu, oldsh, MF_UNCHECKED);
		CheckMenuItem(menu, shape, MF_CHECKED);
		break;

		// 转换客户坐标到屏幕坐标，限制光标只在客户区域移动
	case WM_LBUTTONDOWN:
		GetClientRect(wnd, &C);
		ClientToScreen(wnd, reinterpret_cast<LPPOINT>(&C));
		ClientToScreen(wnd, reinterpret_cast<LPPOINT>(&C) + 1);
		ClipCursor(&C);
		P.x = LOWORD(lp);
		P.y = HIWORD(lp);
		return 0;

	case WM_LBUTTONUP:
		ClipCursor(NULL);
		InvalidateRect(wnd, &target, TRUE);
		return 0;

	case WM_MOUSEMOVE:
		if (wp & MK_LBUTTON)
		{
			// 设置混合模式使得画笔颜色是背景颜色的反色
			// 如果画在先前的矩形上方，那么后者可以被擦除
			SetROP2(hdc, R2_NOTXORPEN);

			//C = target;
			//if (!IsRectEmpty(&target))
			//	Rectangle(hdc, C.left, C.top, C.right, C.bottom);

			// 保存目标矩形的坐标，避免不可用的矩形范围
			POINT T = { LOWORD(lp), HIWORD(lp) };
			C.left = std::min(P.x, T.x);
			C.right = std::max(P.x, T.x);
			C.top = std::min(P.y, T.y);
			C.bottom = std::max(P.y, T.y);
			target = C;

			InvalidateRect(wnd, &C, TRUE);
		}
		return 0;

	case WM_PAINT:
		BeginPaint(wnd, &ps);
		C = target;
		/// 默认刷子是白色的，选个不同的颜色
		SelectObject(hdc, GetStockObject(LTGRAY_BRUSH));
		switch (shape)
		{
		case ID_FILLEDSHAPE_ELLIPSE:
			Ellipse(hdc, C.left, C.top, C.right, C.bottom);
			break;
		case ID_FILLEDSHAPE_RECTANGLE:
			Rectangle(hdc, C.left, C.top, C.right, C.bottom);
			break;
		case ID_FILLEDSHAPE_ROUNDRECT:
			RoundRect(hdc, C.left, C.top, C.right, C.bottom, 10, 10);
			break;
		}
		EndPaint(wnd, &ps);
		return 0;
	}
	return DefWindowProcW(wnd, msg, wp, lp);
}


////////////////////////////////////////////////////////////


int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR cmdline, int cmdshow)
{
	UNREFERENCED_PARAMETER(instance);
	UNREFERENCED_PARAMETER(cmdline);

	CStringW E;
	int const buflen = 1 << 10;
	wchar_t computerName[buflen], userName[buflen];
	DWORD namelen = buflen;
	GetComputerNameW(computerName, &namelen);
	GetUserNameW(userName, &namelen);
	E.Format(L"---------- %s on %s ----------\n",
		userName, computerName);
	OutputDebugStringW(E.GetString());

	HMENU menu = LoadMenuW(instance, MAKEINTRESOURCEW(IDR_MENU1));

	MyWnd m;
	if (!(m.Create(
		L"Bitmap Capture - 你好 ཁམས་བཟང་། こんにちは 안녕하세요",
		WS_OVERLAPPEDWINDOW, 0,
		CW_USEDEFAULT, CW_USEDEFAULT, 600, 600,
		NULL, menu)))
	{
		PopUpError(L"创建窗口失败");
		return -1;
	}

	ShowWindow(m.Get(), cmdshow);
	UpdateWindow(m.Get());

	// Run message loop
	MSG msg = { 0 };
	while (GetMessageW(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return static_cast<int>(msg.wParam);
}