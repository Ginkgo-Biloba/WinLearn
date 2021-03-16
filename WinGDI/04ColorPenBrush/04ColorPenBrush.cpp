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
	RECT edge;

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

	void OnPaint(HDC hdc);
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
	HDC hdc = NULL;
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_LBUTTONDOWN:
	case WM_PAINT:
		hdc = BeginPaint(wnd, &ps);
		OnPaint(hdc);
		EndPaint(wnd, &ps);
		return 0;
	}
	return DefWindowProcW(wnd, msg, wp, lp);
}


void MyWnd::OnPaint(HDC hdc)
{
	// 这里有 GDI 对象内存泄漏的问题
	// DeleteObject 好像没有删除掉

	// 填充客户区区域
	RECT rc, tr;
	GetClientRect(wnd, &rc);
	HRGN rgn = CreateRectRgnIndirect(&rc);
	HBRUSH brush = CreateSolidBrush(RGB(200, 200, 180));
	FillRgn(hdc, rgn, brush);

	// 画笔，画矩形
	DeleteObject(SelectObject(hdc, CreatePen(PS_DOT, 1, RGB(40, 255, 40))));
	SetBkColor(hdc, RGB(64, 64, 64));
	Rectangle(hdc, 10, 10, 200, 200);
	// 文本标题
	SetBkColor(hdc, RGB(220, 220, 230));
	SetRect(&tr, 10, 210, 200, 200);
	DrawTextW(hdc, L"PS_DOT", -1, &tr, DT_CENTER | DT_NOCLIP);

	DeleteObject(SelectObject(hdc, CreatePen(PS_DASHDOTDOT, 1, RGB(40, 230, 230))));
	SetBkColor(hdc, RGB(255, 30, 30));
	DeleteObject(SelectObject(hdc, CreateSolidBrush(RGB(64, 64, 64))));
	Rectangle(hdc, 210, 10, 400, 200);
	SetBkColor(hdc, RGB(240, 200, 200));
	SetRect(&tr, 210, 210, 400, 200);
	DrawTextW(hdc, L"PS_DASHDOTDOT", -1, &tr, DT_CENTER | DT_NOCLIP);

	DeleteObject(SelectObject(hdc, CreatePen(PS_DASHDOT, 1, RGB(240, 30, 30))));
	SetBkColor(hdc, RGB(240, 240, 30));
	DeleteObject(SelectObject(hdc, CreateSolidBrush(RGB(100, 200, 240))));
	Rectangle(hdc, 410, 10, 600, 200);
	SetBkColor(hdc, RGB(200, 240, 200));
	SetRect(&tr, 410, 210, 600, 200);
	DrawTextW(hdc, L"PS_DASHDOT", -1, &tr, DT_CENTER | DT_NOCLIP);

	// 当 penstyle == PS_SOLID，宽度可以超过 1
	// 如果你设置任何画笔的宽度超过 1，那么它画一条实线，即便你尝试选择另外的风格
	DeleteObject(SelectObject(hdc, CreatePen(PS_SOLID, 5, RGB(240, 30, 30))));
	// 当 style == PS_SOLID 时，背景颜色无关紧要
	SetBkColor(hdc, RGB(240, 240, 240));
	DeleteObject(SelectObject(hdc, CreateSolidBrush(RGB(200, 100, 40))));
	Rectangle(hdc, 10, 300, 200, 500);
	SetBkColor(hdc, RGB(200, 200, 240));
	SetRect(&tr, 10, 510, 200, 500);
	DrawTextW(hdc, L"PS_SOLID", -1, &tr, DT_CENTER | DT_NOCLIP);

	DeleteObject(SelectObject(hdc, CreatePen(PS_DASH, 1, RGB(40, 255, 40))));
	SetBkColor(hdc, RGB(64, 64, 64));
	DeleteObject(SelectObject(hdc, CreateSolidBrush(RGB(200, 200, 240))));
	Rectangle(hdc, 210, 300, 400, 500);
	SetBkColor(hdc, RGB(240, 240, 200));
	SetRect(&tr, 210, 510, 400, 200);
	DrawTextW(hdc, L"PS_DASH", -1, &tr, DT_CENTER | DT_NOCLIP);

	DeleteObject(SelectObject(hdc, CreatePen(PS_NULL, 1, RGB(40, 255, 40))));
	// 当 style == PS_NULL 时，背景颜色无关紧要
	SetBkColor(hdc, RGB(64, 64, 64));
	DeleteObject(SelectObject(hdc, CreateSolidBrush(RGB(240, 240, 240))));
	Rectangle(hdc, 410, 300, 600, 500);
	SetBkColor(hdc, RGB(200, 240, 240));
	SetRect(&tr, 410, 510, 600, 500);
	DrawTextW(hdc, L"PS_NULL", -1, &tr, DT_CENTER | DT_NOCLIP);

	DeleteObject(rgn);
	DeleteObject(brush);
	// GetStockObject(WHITE_BRUSH);
	// GetStockObject(DC_PEN);
}


////////////////////////////////////////////////////////////


int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR cmdline, int cmdshow)
{
	UNREFERENCED_PARAMETER(instance);
	UNREFERENCED_PARAMETER(cmdline);

	MyWnd m;
	if (!(m.Create(
		L"ColorPenBrush - 你好 ཁམས་བཟང་། こんにちは 안녕하세요",
		WS_OVERLAPPEDWINDOW, 0,
		CW_USEDEFAULT, CW_USEDEFAULT, 610, 560)))
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