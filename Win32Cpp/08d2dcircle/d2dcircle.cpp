// https://github.com/microsoft/Windows-classic-samples/tree/master/Samples/Win7Samples/begin/LearnWin32/Direct2DClock

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windowsx.h>
#include <atlstr.h>
#include <d2d1.h>
#include <vector>
#include <algorithm>
#undef NDEBUG
#include <cassert>

#include "basewin.hpp"
#include "resource.h"

#pragma comment(lib, "d2d1")
namespace d2d = ::D2D1;
using std::vector;


////////////////////  ////////////////////

struct MyEllipse
{
	D2D1_ELLIPSE ellipse;
	D2D1_COLOR_F color;

	void draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush)
	{
		brush->SetColor(color);
		target->FillEllipse(ellipse, brush);
		brush->SetColor(d2d::ColorF(d2d::ColorF::Black));
		target->DrawEllipse(ellipse, brush, 1.F);
	}

	BOOL hit_test(float x, float y)
	{
		float a = ellipse.radiusX;
		float b = ellipse.radiusY;
		x -= ellipse.point.x;
		y -= ellipse.point.y;
		float d = (x * x) / (a * a) + (y * y) / (b * b);
		return d < 1.F;
	}
};

////////////////////  ////////////////////


class MainWindow : public BaseWindow<MainWindow>
{
	enum ModeType
	{
		MODE_DRAW,
		MODE_DRAG,
		MODE_SELECT,
	};

	CComPtr<ID2D1Factory> factory;
	CComPtr<ID2D1HwndRenderTarget> target;
	CComPtr<ID2D1SolidColorBrush> brush;
	D2D1_POINT_2F A, p2d;
	vector<d2d::ColorF::Enum> colors;
	vector<d2d::ColorF::Enum>::iterator itC;
	vector<MyEllipse> ellipses;
	vector<MyEllipse>::iterator itE;
	int mode;
	HCURSOR cursor;

	D2D1_POINT_2F pixel2dip(int x, int y);

	HRESULT create_graphic();
	void discard_graphic();
	void on_paint();
	void on_size();
	void on_lbtn_down(int x, int y, DWORD flag);
	void on_lbtn_up();
	void on_mouse_move(int x, int y, DWORD flag);
	void on_key_down(unsigned key);

	HRESULT insert_ellipse(D2D_POINT_2F B);
	BOOL hit_test(float x, float y);
	void set_mode(int m);
	void move_select(float x, float y);
	void save_image();

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
	p2d.x = p2d.y = 1.F;
	colors = 
	{
		d2d::ColorF::AliceBlue,
		d2d::ColorF::AntiqueWhite,
		d2d::ColorF::Aqua,
		d2d::ColorF::Aquamarine,
		d2d::ColorF::Azure,
		d2d::ColorF::Beige,
		d2d::ColorF::Bisque,
		// d2d::ColorF::Black,
		d2d::ColorF::BlanchedAlmond,
		d2d::ColorF::Blue,
		d2d::ColorF::BlueViolet,
		d2d::ColorF::Brown,
		d2d::ColorF::BurlyWood,
		d2d::ColorF::CadetBlue,
		d2d::ColorF::Chartreuse,
		d2d::ColorF::Chocolate,
		d2d::ColorF::Coral,
		d2d::ColorF::CornflowerBlue,
		d2d::ColorF::Cornsilk,
		d2d::ColorF::Crimson,
		d2d::ColorF::Cyan,
		d2d::ColorF::DarkBlue,
		d2d::ColorF::DarkCyan,
		d2d::ColorF::DarkGoldenrod,
		d2d::ColorF::DarkGray,
		d2d::ColorF::DarkGreen,
		d2d::ColorF::DarkKhaki,
		d2d::ColorF::DarkMagenta,
		d2d::ColorF::DarkOliveGreen,
		d2d::ColorF::DarkOrange,
		d2d::ColorF::DarkOrchid,
		d2d::ColorF::DarkRed,
		d2d::ColorF::DarkSalmon,
		d2d::ColorF::DarkSeaGreen,
		d2d::ColorF::DarkSlateBlue,
		d2d::ColorF::DarkSlateGray,
		d2d::ColorF::DarkTurquoise,
		d2d::ColorF::DarkViolet,
		d2d::ColorF::DeepPink,
		d2d::ColorF::DeepSkyBlue,
		d2d::ColorF::DimGray,
		d2d::ColorF::DodgerBlue,
		d2d::ColorF::Firebrick,
		d2d::ColorF::FloralWhite,
		d2d::ColorF::ForestGreen,
		d2d::ColorF::Fuchsia,
		d2d::ColorF::Gainsboro,
		d2d::ColorF::GhostWhite,
		d2d::ColorF::Gold,
		d2d::ColorF::Goldenrod,
		d2d::ColorF::Gray,
		d2d::ColorF::Green,
		d2d::ColorF::GreenYellow,
		d2d::ColorF::Honeydew,
		d2d::ColorF::HotPink,
		d2d::ColorF::IndianRed,
		d2d::ColorF::Indigo,
		d2d::ColorF::Ivory,
		d2d::ColorF::Khaki,
		d2d::ColorF::Lavender,
		d2d::ColorF::LavenderBlush,
		d2d::ColorF::LawnGreen,
		d2d::ColorF::LemonChiffon,
		d2d::ColorF::LightBlue,
		d2d::ColorF::LightCoral,
		d2d::ColorF::LightCyan,
		d2d::ColorF::LightGoldenrodYellow,
		d2d::ColorF::LightGreen,
		d2d::ColorF::LightGray,
		d2d::ColorF::LightPink,
		d2d::ColorF::LightSalmon,
		d2d::ColorF::LightSeaGreen,
		d2d::ColorF::LightSkyBlue,
		d2d::ColorF::LightSlateGray,
		d2d::ColorF::LightSteelBlue,
		d2d::ColorF::LightYellow,
		d2d::ColorF::Lime,
		d2d::ColorF::LimeGreen,
		d2d::ColorF::Linen,
		d2d::ColorF::Magenta,
		d2d::ColorF::Maroon,
		d2d::ColorF::MediumAquamarine,
		d2d::ColorF::MediumBlue,
		d2d::ColorF::MediumOrchid,
		d2d::ColorF::MediumPurple,
		d2d::ColorF::MediumSeaGreen,
		d2d::ColorF::MediumSlateBlue,
		d2d::ColorF::MediumSpringGreen,
		d2d::ColorF::MediumTurquoise,
		d2d::ColorF::MediumVioletRed,
		d2d::ColorF::MidnightBlue,
		d2d::ColorF::MintCream,
		d2d::ColorF::MistyRose,
		d2d::ColorF::Moccasin,
		d2d::ColorF::NavajoWhite,
		d2d::ColorF::Navy,
		d2d::ColorF::OldLace,
		d2d::ColorF::Olive,
		d2d::ColorF::OliveDrab,
		d2d::ColorF::Orange,
		d2d::ColorF::OrangeRed,
		d2d::ColorF::Orchid,
		d2d::ColorF::PaleGoldenrod,
		d2d::ColorF::PaleGreen,
		d2d::ColorF::PaleTurquoise,
		d2d::ColorF::PaleVioletRed,
		d2d::ColorF::PapayaWhip,
		d2d::ColorF::PeachPuff,
		d2d::ColorF::Peru,
		d2d::ColorF::Pink,
		d2d::ColorF::Plum,
		d2d::ColorF::PowderBlue,
		d2d::ColorF::Purple,
		// d2d::ColorF::Red,
		d2d::ColorF::RosyBrown,
		d2d::ColorF::RoyalBlue,
		d2d::ColorF::SaddleBrown,
		d2d::ColorF::Salmon,
		d2d::ColorF::SandyBrown,
		d2d::ColorF::SeaGreen,
		d2d::ColorF::SeaShell,
		d2d::ColorF::Sienna,
		d2d::ColorF::Silver,
		// d2d::ColorF::SkyBlue,
		d2d::ColorF::SlateBlue,
		d2d::ColorF::SlateGray,
		d2d::ColorF::Snow,
		d2d::ColorF::SpringGreen,
		d2d::ColorF::SteelBlue,
		d2d::ColorF::Tan,
		d2d::ColorF::Teal,
		d2d::ColorF::Thistle,
		d2d::ColorF::Tomato,
		d2d::ColorF::Turquoise,
		d2d::ColorF::Violet,
		d2d::ColorF::Wheat,
		d2d::ColorF::White,
		d2d::ColorF::WhiteSmoke,
		d2d::ColorF::Yellow,
		d2d::ColorF::YellowGreen,
	};
	itC = colors.begin();
	itE = ellipses.end();
	set_mode(MODE_DRAW);
}


D2D1_POINT_2F MainWindow::pixel2dip(int x, int y)
{
	D2D1_POINT_2F p;
	p.x = p2d.x * x;
	p.y = p2d.y * y;
	return p;
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


void MainWindow::on_paint()
{
	HRESULT ret = create_graphic();
	if (FAILED(ret))
		return;

	PAINTSTRUCT ps;
	BeginPaint(wnd, &ps);
	target->BeginDraw();

	target->Clear(d2d::ColorF(d2d::ColorF::SkyBlue));
	for (auto& e : ellipses)
		e.draw(target, brush);
	if (itE != ellipses.end())
	{
		brush->SetColor(d2d::ColorF(d2d::ColorF::Red));
		target->DrawEllipse(itE->ellipse, brush, 2.0);
	}

	ret = target->EndDraw();
	if (FAILED(ret) || (ret == D2DERR_RECREATE_TARGET))
		discard_graphic();
	EndPaint(wnd, &ps);
}


void MainWindow::on_size()
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
	D2D1_POINT_2F B = pixel2dip(x, y);
	if (mode == MODE_DRAW)
	{
		POINT pt = { x, y };
		if (DragDetect(wnd, pt))
		{
			SetCapture(wnd);
			insert_ellipse(B);
		}
	}
	else if (hit_test(B.x, B.y))
	{
		SetCapture(wnd);
		A = itE->ellipse.point;
		A.x -= B.x;
		A.y -= B.y;
		set_mode(MODE_DRAG);
	}
	InvalidateRect(wnd, NULL, FALSE);
}


void MainWindow::on_lbtn_up()
{
	if (mode == MODE_DRAW && itE != ellipses.end())
	{
		itE = ellipses.end();
		InvalidateRect(wnd, NULL, FALSE);
	}
	else if (mode == MODE_DRAG)
		set_mode(MODE_SELECT);
	ReleaseCapture();
}


void MainWindow::on_mouse_move(int x, int y, DWORD flag)
{
	D2D1_POINT_2F B = pixel2dip(x, y);
	if (!(flag & MK_LBUTTON) || (itE == ellipses.end()))
		return;

	if (mode == MODE_DRAW)
	{
		float Rx = (B.x - A.x) * 0.5F;
		float Ry = (B.y - A.y) * 0.5F;
		// center
		B.x = A.x + Rx;
		B.y = A.y + Ry;
		itE->ellipse = d2d::Ellipse(B, Rx, Ry);
	}
	else if (mode == MODE_DRAG)
	{
		itE->ellipse.point
			= d2d::Point2F(A.x + B.x, A.y + B.y);
	}
	InvalidateRect(wnd, NULL, FALSE);
}


void MainWindow::on_key_down(unsigned key)
{
	switch (key)
	{
	case VK_BACK:
	case VK_DELETE:
		if ((mode == MODE_SELECT) && itE != ellipses.end())
		{
			// 应该用 list 的
			ellipses.erase(itE);
			itE = ellipses.end();
			set_mode(MODE_SELECT);
			InvalidateRect(wnd, NULL, FALSE);
		}
		break;
	case VK_LEFT:
		move_select(-1, 0);
		break;
	case VK_RIGHT:
		move_select(1, 0);
		break;
	case VK_UP:
		move_select(0, -1);
		break;
	case VK_DOWN:
		move_select(0, 1);
		break;
	case VK_F4:
		save_image();
		break;
	}
}


LRESULT MainWindow::handle_message(UINT msg, WPARAM wp, LPARAM lp)
{
	PMINMAXINFO mmi;
	switch (msg)
	{
	case WM_CREATE:
		if (FAILED(D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory)))
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
		on_paint();
		return 0;

	case WM_SIZE:
		on_size();
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

	case WM_SETCURSOR:
		if (LOWORD(lp) == HTCLIENT)
		{
			SetCursor(cursor);
			return TRUE;
		}
		break;

	case WM_KEYDOWN:
		on_key_down(static_cast<unsigned>(wp));
		return 0;

	case WM_COMMAND:
		OutputDebugStringW(L"WM_COMMAND\n");
		switch (LOWORD(wp))
		{
		case ID_MODE_DRAW:
			set_mode(MODE_DRAW);
			break;
		case ID_MODE_SELECT:
			set_mode(MODE_SELECT);
			break;
		case ID_MODE_TOGGLE:
			if (mode == MODE_DRAW)
				set_mode(MODE_SELECT);
			else
				set_mode(MODE_DRAW);
			break;
		}
		return 0;
	}

	return DefWindowProcW(wnd, msg, wp, lp);
}


HRESULT MainWindow::insert_ellipse(D2D_POINT_2F B)
{
	MyEllipse e;
	B.x -= 1;
	B.y -= 1;
	e.ellipse.point = A = B;
	e.ellipse.radiusX = e.ellipse.radiusY = 1.F;
	e.color = d2d::ColorF(*itC);

	ellipses.push_back(e);
	itE = --(ellipses.end());
	if (++itC == colors.end())
		itC = colors.begin();
	return S_OK;
}


BOOL MainWindow::hit_test(float x, float y)
{
	for (auto it = ellipses.end(); it != ellipses.begin();)
	{
		--it;
		if (it->hit_test(x, y))
		{
			itE = it;
			return TRUE;
		}
	}
	itE = ellipses.end();
	return FALSE;
}


void MainWindow::set_mode(int m)
{
	mode = m;
	wchar_t* hc = nullptr;
	switch (mode)
	{
	case MODE_DRAW:   hc = IDC_CROSS; break;
	case MODE_DRAG:   hc = IDC_SIZEALL; break;
	case MODE_SELECT: hc = IDC_HAND; break;
	}
	cursor = LoadCursorW(NULL, hc);
	SetCursor(cursor);
}


void MainWindow::move_select(float x, float y)
{
	if ((mode == MODE_SELECT) && (itE != ellipses.end()))
	{
		itE->ellipse.point.x += x;
		itE->ellipse.point.y += y;
		InvalidateRect(wnd, NULL, FALSE);
	}
}


void MainWindow::save_image()
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	CStringW name;
	name.Format(L"D2DCircle-%4hu%02hu%02hu%02hu%02hu%02hu.bmp\n",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	OutputDebugStringW(name.GetString());
	name.Truncate(name.GetLength() - 1);
	/*
	D2D1_BITMAP_PROPERTIES prop = d2d::BitmapProperties(
		d2d::PixelFormat(DXGI_FORMAT_B8G8R8X8_TYPELESS, D2D1_ALPHA_MODE_IGNORE));
	CComPtr<ID2D1Bitmap> dst;
	HRESULT ret = target->CreateBitmap(target->GetPixelSize(), NULL, 0, prop, &dst);
	D2D1_PIXEL_FORMAT pixel = target->GetPixelFormat();
	*/
}

////////////////////  ////////////////////


int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int cmdshow)
{
	MainWindow mw;
	if (!(mw.create(
		L"D2D Circle - 你好 ཁམས་བཟང་། こんにちは 안녕하세요",
		WS_OVERLAPPEDWINDOW, 0,
		CW_USEDEFAULT, CW_USEDEFAULT, 600, 600)))
	{
		popup_error();
		return -1;
	}

	HACCEL accel = LoadAcceleratorsW(instance, MAKEINTRESOURCEW(IDR_ACCEL1));

	ShowWindow(mw.data(), cmdshow);

	// Run message loop
	MSG msg = { 0 };
	while (GetMessageW(&msg, NULL, 0, 0))
		if (!TranslateAcceleratorW(mw.data(), accel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

	return 0;
}