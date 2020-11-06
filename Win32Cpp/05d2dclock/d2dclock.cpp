// https://github.com/microsoft/Windows-classic-samples/tree/master/Samples/Win7Samples/begin/LearnWin32/Direct2DClock

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <atlstr.h>
#include <d2d1.h>
#include <algorithm>
#undef NDEBUG
#include <cassert>

#include "basewin.hpp"
#include "scene.hpp"

#pragma comment(lib, "d2d1")
namespace d2d = ::D2D1;


////////////////////  ////////////////////


class Clock : public GraphicScene
{
	CComPtr<ID2D1SolidColorBrush> brushfill;
	CComPtr<ID2D1SolidColorBrush> brushstroke;

	D2D1_ELLIPSE ellipse;
	D2D1_POINT_2F dial[120];

	virtual HRESULT create_device_independent_resource() override { return S_OK; };
	virtual HRESULT release_device_independent_resource() override { return S_OK; };
	virtual HRESULT create_device_dependent_resource() override;
	virtual HRESULT release_device_dependent_resource() override;
	virtual void calculate_layout() override;
	virtual void render_scene() override;

	void draw_hand(float len, float angle, float strokewidth);
};


HRESULT Clock::create_device_dependent_resource()
{
	HRESULT ret = target->CreateSolidColorBrush(
		d2d::ColorF(d2d::ColorF::DarkSeaGreen),
		d2d::BrushProperties(),
		&brushfill);
	if (SUCCEEDED(ret))
	{
		ret = target->CreateSolidColorBrush(
			d2d::ColorF(d2d::ColorF::Black),
			d2d::BrushProperties(),
			&brushstroke);
	}
	return ret;
}


HRESULT Clock::release_device_dependent_resource()
{
	brushfill.Release();
	brushstroke.Release();
	return S_OK;
}


void Clock::calculate_layout()
{
	D2D1_SIZE_F sz = target->GetSize();
	float x = sz.width * 0.5F;
	float y = sz.height * 0.5F;
	float r = std::min(x, y);
	ellipse = d2d::Ellipse(d2d::Point2F(x, y), r, r);

	D2D1_POINT_2F p0 = d2d::Point2F(x, y - r);
	D2D1_POINT_2F p1 = d2d::Point2F(x, y - r * 0.95F);
	D2D1_POINT_2F p2 = d2d::Point2F(x, y - r * 0.85F);
	d2d::Matrix3x2F m;
	for (int i = 0; i < 120; i += 2)
	{
		m = d2d::Matrix3x2F::Rotation(360.F / 60 * (i / 2), ellipse.point);
		dial[i] = m.TransformPoint(p0);
		if (i % 10)
			dial[i + 1] = m.TransformPoint(p1);
		else
			dial[i + 1] = m.TransformPoint(p2);
	}
}


void Clock::render_scene()
{
	target->Clear(d2d::ColorF(d2d::ColorF::CadetBlue));

#if 0
	CComPtr<ID2D1RadialGradientBrush> brushrad;
	D2D1_GRADIENT_STOP grad[] =
	{
		d2d::GradientStop(0.0, d2d::ColorF(d2d::ColorF::CadetBlue)),
		d2d::GradientStop(1.0, d2d::ColorF(d2d::ColorF::DarkSeaGreen))
	};
	CComPtr< ID2D1GradientStopCollection> coll;
	D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES prop =
		d2d::RadialGradientBrushProperties(ellipse.point, d2d::Point2F(), ellipse.radiusX, ellipse.radiusY);
	HRESULT ret = target->CreateGradientStopCollection(grad, 2, &coll);
	if (SUCCEEDED(ret))
		ret = target->CreateRadialGradientBrush(prop, coll, &brushrad);
	if (SUCCEEDED(ret))
		target->FillEllipse(ellipse, brushrad);
	else
#endif
		target->FillEllipse(ellipse, brushfill);
	target->DrawEllipse(ellipse, brushstroke);

	for (int i = 0; i < 120; i += 2)
		target->DrawLine(dial[i], dial[i + 1], brushstroke, 2.F);

	SYSTEMTIME t;
	GetLocalTime(&t);

	float S = t.wSecond + t.wMilliseconds * 1e-3F;
	float M = t.wMinute + t.wSecond / 60.F;
	float H = t.wHour + t.wMinute / 60.F;
	draw_hand(0.5F, 360.F / 12.F * H, 5.F);
	draw_hand(0.7F, 360.F / 60.F * M, 3.F);
	draw_hand(0.9F, 360.F / 60.F * S, 1.F);

	target->SetTransform(d2d::Matrix3x2F::Identity());
	target->FillEllipse(d2d::Ellipse(ellipse.point, 2, 2), brushstroke);
}


void Clock::draw_hand(float len, float angle, float strokewidth)
{
	target->SetTransform(d2d::Matrix3x2F::Rotation(angle, ellipse.point));
	D2D_POINT_2F p0 = d2d::Point2F(
		ellipse.point.x,
		ellipse.point.y - (ellipse.radiusY * len));
	D2D_POINT_2F p1 = d2d::Point2F(
		ellipse.point.x,
		ellipse.point.y + (ellipse.radiusY * 0));
	target->DrawLine(p0, p1, brushstroke, strokewidth);
}


////////////////////  ////////////////////


class MainWindow : public BaseWindow<MainWindow>
{
	HANDLE timer;
	Clock scene;

	BOOL init_timer();

public:
	MainWindow() : timer(NULL) {}

	void wait_timer();

	wchar_t const* class_name() const override { return L"MainWindowClock"; };
	LRESULT handle_message(UINT msg, WPARAM wp, LPARAM lp) override;
};


BOOL MainWindow::init_timer()
{
	timer = CreateWaitableTimerW(NULL, FALSE, NULL);
	if (!timer)
		return FALSE;

	LARGE_INTEGER li = { 0 };
	if (!SetWaitableTimer(timer, &li, 33, NULL, NULL, FALSE))
	{
		CloseHandle(timer);
		timer = NULL;
		return FALSE;
	}
	return TRUE;
}


void MainWindow::wait_timer()
{
	// Wait until the timer expires or any message is posted.
	if (MsgWaitForMultipleObjects(1, &timer, FALSE, INFINITE, QS_ALLINPUT)
		== WAIT_OBJECT_0)
		InvalidateRect(wnd, NULL, FALSE);
}


LRESULT MainWindow::handle_message(UINT msg, WPARAM wp, LPARAM lp)
{
	PMINMAXINFO mmi;
	switch (msg)
	{
	case WM_CREATE:
	{
		if (FAILED(scene.create()))
		{
			popup_error();
			return -1;
		}
		if (init_timer())
			return TRUE;
		else
		{
			popup_error();
			return -1;
		}
	}

	case WM_DESTROY:
		CloseHandle(timer);
		scene.release();
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
	case WM_DISPLAYCHANGE:
	{
		PAINTSTRUCT ps = { 0 };
		BeginPaint(wnd, &ps);
		scene.render(wnd);
		EndPaint(wnd, &ps);
		return 0;
	}

	case WM_SIZE:
	{
		int x = LOWORD(lp);
		int y = HIWORD(lp);
		scene.resize(x, y);
		InvalidateRect(wnd, NULL, FALSE);
		return 0;
	}

	case WM_GETMINMAXINFO:
		mmi = reinterpret_cast<PMINMAXINFO>(lp);
		mmi->ptMinTrackSize.x = 200 + winedge;
		mmi->ptMinTrackSize.y = 200 + winedge + winhcap;
		//mmi->ptMaxTrackSize.x = 1600 + winedge;
		//mmi->ptMaxTrackSize.y = 1000 + winedge + winhcap;
		return 0;

	case WM_ERASEBKGND:
		return 1;

	default:
		return DefWindowProcW(wnd, msg, wp, lp);
	}
}


////////////////////  ////////////////////


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR cmdline, int cmdshow)
{
	_CRT_UNUSED((hInstance || cmdline));

	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
		return 1;

	MainWindow m;
	if (!(m.create(
		L"clock 时钟 時計 시계",
		WS_OVERLAPPEDWINDOW)))
		return 2;

	ShowWindow(m.data(), cmdshow);

	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
			continue;
		}
		m.wait_timer();
	}

	CoUninitialize();
	return 0;
}

