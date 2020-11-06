#define WIN32_LEAN_AND_MEAN
#include <atlstr.h>
#include <d2d1.h>
#undef NDEBUG
#include <cassert>

#pragma comment(lib, "d2d1")


template <class T>
class BaseWindow
{
protected:
	HWND hwnd;
	int winhcap;
	int winedge;

	virtual wchar_t const* class_name() const = 0;
	virtual LRESULT handle_message(UINT, WPARAM, LPARAM) = 0;

public:
	static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
	{
		T* t = NULL;
		if (msg == WM_NCCREATE)
		{
			CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
			t = reinterpret_cast<T*>(cs->lpCreateParams);
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(t));
			t->hwnd = hwnd;
		}
		else
			t = reinterpret_cast<T*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

		return t
			? t->handle_message(msg, wp, lp)
			: DefWindowProcW(hwnd, msg, wp, lp);
	}

	BaseWindow() : hwnd(NULL) {}

	virtual ~BaseWindow() {};

	BOOL create_window(
		wchar_t const* winname,
		DWORD style,
		DWORD exstyle = 0,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		int width = CW_USEDEFAULT,
		int height = CW_USEDEFAULT,
		HWND parent = NULL,
		HMENU menu = NULL
	)
	{
		WNDCLASS wc = { 0 };
		wc.lpfnWndProc = T::window_proc;
		wc.hInstance = GetModuleHandleW(NULL);
		wc.lpszClassName = class_name();
		RegisterClassW(&wc);

		winhcap = GetSystemMetrics(SM_CYCAPTION);
		winedge = 16;
		if (width == CW_USEDEFAULT)
			width = 600;
		if (height == CW_USEDEFAULT)
			height = 600;

		hwnd = CreateWindowExW(
			exstyle, class_name(), winname, style,
			x, y, width + winedge, height + winedge + winhcap,
			parent, menu, wc.hInstance, this);
		return hwnd ? TRUE : FALSE;
	}

	HWND native() const
	{
		return hwnd;
	}
};


class MainWindow : public BaseWindow<MainWindow>
{
	CComPtr<ID2D1Factory> factory;
	CComPtr<ID2D1HwndRenderTarget> target;
	CComPtr<ID2D1SolidColorBrush> brush;
	D2D1_ELLIPSE ellipse;

	void layout();
	HRESULT create_graphic();
	void discard_graphic();
	void onpaint();
	void resize();

public:
	wchar_t const* class_name() const override
	{
		return L"MainWindow";
	}

	LRESULT handle_message(UINT msg, WPARAM wparam, LPARAM lparam) override;

	~MainWindow() { discard_graphic(); }
};


// Recalculate drawing layout when the size of the window changes.
void MainWindow::layout()
{
	if (!target)
		return;

	D2D1_SIZE_F size = target->GetSize();
	float x = size.width / 2.F;
	float y = size.height / 2.F;
	float rad = min(x, y);
	ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), rad, rad);
}


HRESULT MainWindow::create_graphic()
{
	HRESULT ret = S_OK;
	if (!!target)
		return ret;

	RECT rc;
	GetClientRect(hwnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
	ret = factory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		D2D1::HwndRenderTargetProperties(hwnd, size),
		&target);
	if (SUCCEEDED(ret))
	{
		D2D1_COLOR_F color = D2D1::ColorF(D2D1::ColorF::Pink);
		ret = target->CreateSolidColorBrush(color, &brush);
		if (SUCCEEDED(ret))
			layout();
	}
	return ret;
}


void MainWindow::discard_graphic()
{
	target.Release();
	brush.Release();
}


void MainWindow::onpaint()
{
	HRESULT ret = create_graphic();
	if (FAILED(ret))
		return;

	PAINTSTRUCT ps;
	BeginPaint(hwnd, &ps);
	target->BeginDraw();
	target->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));
	target->FillEllipse(ellipse, brush);
	ret = target->EndDraw();
	if (FAILED(ret) || (ret == D2DERR_RECREATE_TARGET))
		discard_graphic();
	EndPaint(hwnd, &ps);
}


void MainWindow::resize()
{
	if (!target)
		return;

	RECT rc;
	GetClientRect(hwnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
	target->Resize(size);
	layout();
	InvalidateRect(hwnd, NULL, FALSE);
}


LRESULT MainWindow::handle_message(UINT msg, WPARAM wparam, LPARAM lparam)
{
	PMINMAXINFO lp;
	switch (msg)
	{
	case WM_CREATE:
		if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory)))
			return -1;
		return 0;

	case WM_DESTROY:
		discard_graphic();
		factory.Release();
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
		onpaint();
		return 0;

	case WM_SIZE:
		resize();
		return 0;

	case WM_GETMINMAXINFO:
		lp = reinterpret_cast<PMINMAXINFO>(lparam);
		lp->ptMinTrackSize.x = 200 + winedge;
		lp->ptMinTrackSize.y = 200 + winedge + winhcap;
		lp->ptMaxTrackSize.x = 1600 + winedge;
		lp->ptMaxTrackSize.y = 1000 + winedge + winhcap;
		return 0;
	}

	return DefWindowProcW(hwnd, msg, wparam, lparam);
}


int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int cmdshow)
{
	MainWindow mw;
	assert(mw.create_window(
		L"D2D Circle - 你好 ཁམས་བཟང་། こんにちは 안녕하세요",
		WS_OVERLAPPEDWINDOW, 0,
		CW_USEDEFAULT, CW_USEDEFAULT, 400, 400));

	ShowWindow(mw.native(), cmdshow);

	// Run message loop
	MSG msg = { 0 };
	while (GetMessageW(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return 0;
}

