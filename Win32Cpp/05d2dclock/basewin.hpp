#pragma once


void popup_error()
{
	wchar_t* msg = NULL;
	DWORD err = GetLastError();
	DWORD len = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, err, 0 /*MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)*/,
		reinterpret_cast<wchar_t*>(&msg), 0, NULL);
	if (len)
	{
		MessageBoxW(NULL, msg, L"错误", MB_OK);
		LocalFree(msg);
	}
	else
		MessageBoxW(NULL, L"未知错误", L"错误", MB_OK);
}


template <class T>
class BaseWindow
{
protected:
	HWND wnd;
	int winhcap, winedge;

	virtual wchar_t const* class_name() const = 0;
	virtual LRESULT handle_message(UINT msg, WPARAM wp, LPARAM lp) = 0;

public:
	BaseWindow() : wnd(NULL) {}

	virtual ~BaseWindow() { /* if (wnd) CloseWindow(wnd); */ };

	HWND data() const { return wnd; }

	BOOL create(
		wchar_t const* title,
		DWORD style, DWORD exstyle = 0,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		int width = 600,
		int height = 600,
		HWND parent = NULL,
		HMENU menu = NULL
	);

	static LRESULT CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
};



template<class T>
inline BOOL BaseWindow<T>::create(
	wchar_t const* title, DWORD style, DWORD exstyle,
	int x, int y, int width, int height,
	HWND parent, HMENU menu)
{
	WNDCLASSW wc = { 0 };
	wc.lpfnWndProc = BaseWindow::WindowProc;
	wc.hInstance = GetModuleHandleW(NULL);
	wc.lpszClassName = class_name();
	RegisterClassW(&wc);

	winhcap = GetSystemMetrics(SM_CYCAPTION);
	winedge = 16;
	CreateWindowExW(exstyle, class_name(), title, style,
		x, y, width + winedge, height + winedge + winhcap,
		parent, menu, GetModuleHandleW(NULL), this);

	if (wnd)
		return TRUE;
	else
	{
		popup_error();
		return FALSE;
	}
}


template <class T>
LRESULT BaseWindow<T>::WindowProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	T* m = NULL;

	if (msg == WM_NCCREATE)
	{
		CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
		m = reinterpret_cast<T*>(cs->lpCreateParams);
		SetWindowLongPtrW(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(m));
		m->wnd = wnd;
	}
	else
		m = reinterpret_cast<T*>(GetWindowLongPtrW(wnd, GWLP_USERDATA));

	return m
		? m->handle_message(msg, wp, lp)
		: DefWindowProcW(wnd, msg, wp, lp);
}



