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


static BOOL GetDisplayMonitorInfo(int devidx)
{
	DISPLAY_DEVICEW disdev = { 0 };
	disdev.cb = sizeof(disdev);

	HINSTANCE instance = LoadLibraryW(L"user32.dll");
	if (!instance)
		return FALSE;

	BOOL ret = EnumDisplayDevicesW(NULL, devidx, &disdev, 0);
	if (ret)
		EnumDisplayDevicesW(disdev.DeviceName, 0, &disdev, 0);
	FreeLibrary(instance);
	return ret;
}


////////////////////////////////////////////////////////////


class MyWnd
{
	static LRESULT CALLBACK WinProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

protected:
	HWND wnd;
	RECT edge;
	int method;

	wchar_t const* ClassName() const { return L"MyWnd"; };
	LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp);
	void OnPaint(HDC hdc);

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
	method = 0;
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
		InvalidateRect(wnd, NULL, TRUE);
		return 0;
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
	XFORM xform;
	RECT rc;

	/*
	设置映射模式 LOENGLISH
	这将把客户区域原点从窗口左上角移动到窗口左下角
	同时翻转 y 轴使得画图操作在笛卡儿坐标系中正确进行
	保证比例以便对象绘制在不同显示器上保持大小
	*/
	SetGraphicsMode(hdc, GM_ADVANCED);
	SetMapMode(hdc, MM_LOENGLISH);

	switch (method)
	{
	case 0: // normal
		xform = { 1, 0, 0, 1, 0, 0 };
		break;
	case 1: // scale
		xform = { 0.5F, 0.0, 0.0, 0.5F, 0, 0 };
		break;
	case 2: // translate
		xform = { 1, 0, 0, 1, 75, 0 };
		break;
	case 3: // rotate
		xform = { cosf(30), sinf(30), -sinf(30), cosf(30), 0, 0 };
		break;
	case 4: // shear, along x-axis with proportionality 1.0
		xform = { 1, 1, 0, 1, 0, 0 };
		break;
	case 5: // reflect about horizontal axis
		xform = { 1, 0, 0, -1, 0, 0 };
		break;
	default:
		break;
	}
	method = (method + 1) % 6;
	SetWorldTransform(hdc, &xform);

	GetClientRect(wnd, &rc);
	DPtoLP(hdc, reinterpret_cast<LPPOINT>(&rc), 2);
	SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
	POINT m = { rc.right / 2, rc.bottom / 2 };

	Ellipse(hdc, m.x - 100, m.y + 100, m.x + 100, m.y - 100);
	Ellipse(hdc, m.x - 94, m.y + 94, m.x + 94, m.y - 94);
	Rectangle(hdc, m.x - 13, m.y + 113, m.x + 13, m.y + 50);
	Rectangle(hdc, m.x - 13, m.y + 96, m.x + 13, m.y + 50);

	MoveToEx(hdc, m.x - 150, m.y, NULL);
	LineTo(hdc, m.x - 16, m.y);
	MoveToEx(hdc, m.x - 13, m.y, NULL);
	LineTo(hdc, m.x + 13, m.y);
	MoveToEx(hdc, m.x + 16, m.y, NULL);
	LineTo(hdc, m.x + 150, m.y);

	MoveToEx(hdc, m.x, m.y - 150, NULL);
	LineTo(hdc, m.x, m.y - 16);
	MoveToEx(hdc, m.x, m.y - 13, NULL);
	LineTo(hdc, m.x, m.y + 13);
	MoveToEx(hdc, m.x, m.y + 16, NULL);
	LineTo(hdc, m.x, m.y + 150);
}


////////////////////////////////////////////////////////////


int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR cmdline, int cmdshow)
{
	UNREFERENCED_PARAMETER(instance);
	UNREFERENCED_PARAMETER(cmdline);
	GetDisplayMonitorInfo(0);

	MyWnd m;
	if (!(m.Create(
		L"Coordinate - 你好 ཁམས་བཟང་། こんにちは 안녕하세요",
		WS_OVERLAPPEDWINDOW, 0,
		CW_USEDEFAULT, CW_USEDEFAULT, 500, 500)))
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

	return msg.message;
}
