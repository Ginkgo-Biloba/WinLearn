#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <atlstr.h>
#include <d2d1.h>
#include <windowsx.h>
#include <algorithm>
#undef NDEBUG
#include <cassert>

#pragma comment(lib, "d2d1")
namespace d2d = ::D2D1;

template <class T>
class BaseWindow
{
protected:
	HWND wnd;
	int winhcap;
	int winedge;

	virtual wchar_t const* class_name() const = 0;
	virtual LRESULT handle_message(UINT, WPARAM, LPARAM) = 0;

public:
	static LRESULT CALLBACK window_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
	{
		T* t = NULL;
		if (msg == WM_NCCREATE)
		{
			CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
			t = reinterpret_cast<T*>(cs->lpCreateParams);
			SetWindowLongPtrW(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(t));
			t->wnd = wnd;
		}
		else
			t = reinterpret_cast<T*>(GetWindowLongPtr(wnd, GWLP_USERDATA));

		return t
			? t->handle_message(msg, wp, lp)
			: DefWindowProcW(wnd, msg, wp, lp);
	}

	BaseWindow() : wnd(NULL) {}

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

		CreateWindowExW(
			exstyle, class_name(), winname, style,
			x, y, width + winedge, height + winedge + winhcap,
			parent, menu, wc.hInstance, this);
		return wnd ? TRUE : FALSE;
	}

	HWND native() const
	{
		return wnd;
	}
};


class MainWindow : public BaseWindow<MainWindow>
{
	CComPtr<ID2D1Factory> factory;
	CComPtr<ID2D1HwndRenderTarget> target;
	CComPtr<ID2D1SolidColorBrush> brush;
	D2D1_ELLIPSE ellipse;
	D2D1_POINT_2F A, p2d;

	HRESULT create_graphic();
	void discard_graphic();
	void onpaint();
	void resize();
	void on_lbtn_down(int x, int y, DWORD flag);
	void on_lbtn_up();
	void on_mouse_move(int x, int y, DWORD flag);
	D2D1_POINT_2F pixel2dip(int x, int y);

public:
	wchar_t const* class_name() const override
	{
		return L"MainWindow";
	}

	LRESULT handle_message(UINT msg, WPARAM wp, LPARAM lp) override;

	MainWindow();
};


MainWindow::MainWindow()
{
	A = d2d::Point2F(0, 0);
	ellipse = d2d::Ellipse(A, 0, 0);
	p2d.x = p2d.y = 1.F;
}


HRESULT MainWindow::create_graphic()
{
	HRESULT ret = S_OK;
	if (!!target)
		return ret;

	RECT rc;
	GetClientRect(wnd, &rc);
	D2D1_SIZE_U size = d2d::SizeU(rc.right, rc.bottom);
	ret = factory->CreateHwndRenderTarget(
		d2d::RenderTargetProperties(),
		d2d::HwndRenderTargetProperties(wnd, size),
		&target);
	if (SUCCEEDED(ret))
	{
		D2D1_COLOR_F color = d2d::ColorF(d2d::ColorF::Pink);
		ret = target->CreateSolidColorBrush(color, &brush);
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
	BeginPaint(wnd, &ps);
	target->BeginDraw();
	target->Clear(d2d::ColorF(d2d::ColorF::SkyBlue));
	target->FillEllipse(ellipse, brush);
	ret = target->EndDraw();
	if (FAILED(ret) || (ret == D2DERR_RECREATE_TARGET))
		discard_graphic();
	EndPaint(wnd, &ps);
}


void MainWindow::resize()
{
	if (!target)
		return;

	RECT rc;
	GetClientRect(wnd, &rc);
	D2D1_SIZE_U size = d2d::SizeU(rc.right, rc.bottom);
	target->Resize(size);
	InvalidateRect(wnd, NULL, FALSE);
}


void MainWindow::on_lbtn_down(int x, int y, DWORD)
{
	SetCapture(wnd);
	ellipse.point = A = pixel2dip(x ,y);
	ellipse.radiusX = ellipse.radiusY = 1.F;
	InvalidateRect(wnd, NULL, FALSE);
}


void MainWindow::on_lbtn_up()
{
	ReleaseCapture();
}


void MainWindow::on_mouse_move(int x, int y, DWORD flag)
{
	if (!(flag & MK_LBUTTON))
		return;

	D2D1_POINT_2F B = pixel2dip(x, y);
#if 1
	// semi-axis length
	float Rx = (B.x - A.x) * 0.5F;
	float Ry = (B.y - A.y) * 0.5F;
	// center
	B.x = A.x + Rx;
	B.y = A.y + Ry;
	ellipse = d2d::Ellipse(B, Rx, Ry);
#else 
	ellipse = d2d::Ellipse(A, B.x - A.x, B.y - A.y);
#endif
	InvalidateRect(wnd, NULL, FALSE);
}


D2D1_POINT_2F MainWindow::pixel2dip(int x, int y)
{
	D2D1_POINT_2F p;
	p.x = p2d.x * x;
	p.y = p2d.y * y;
	return p;
}


LRESULT MainWindow::handle_message(UINT msg, WPARAM wp, LPARAM lp)
{
	PMINMAXINFO mmi;
	switch (msg)
	{
	case WM_CREATE:
		if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory)))
			return -1;
		factory->GetDesktopDpi(&p2d.x, &p2d.y);
		p2d.x = 96.F / p2d.x;
		p2d.y = 96.F / p2d.y;
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
		mmi = reinterpret_cast<PMINMAXINFO>(lp);
		mmi->ptMinTrackSize.x = 200 + winedge;
		mmi->ptMinTrackSize.y = 200 + winedge + winhcap;
		//mmi->ptMaxTrackSize.x = 1600 + winedge;
		//mmi->ptMaxTrackSize.y = 1000 + winedge + winhcap;
		return 0;

	case WM_LBUTTONDOWN:
		on_lbtn_down(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), static_cast<DWORD>(wp));
		return 0;

	case WM_LBUTTONUP:
		on_lbtn_up();
		return 0;

	case WM_MOUSEMOVE:
		on_mouse_move(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), static_cast<DWORD>(wp));
		return 0;
	}

	return DefWindowProcW(wnd, msg, wp, lp);
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

