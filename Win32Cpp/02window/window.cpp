#define WIN32_LEAN_AND_MEAN
#include <atlstr.h>
#undef NDEBUG
#include <cassert>


template <class T>
class BaseWindow
{
protected:
	HWND handle;

	virtual wchar_t const* ClassName() const = 0;
	virtual LRESULT HandleMessage(UINT, WPARAM, LPARAM) = 0;

public:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
	{
		T* t = NULL;
		if (msg == WM_NCCREATE)
		{
			CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
			t = reinterpret_cast<T*>(cs->lpCreateParams);
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(t));
			t->handle = hwnd;
		}
		else
			t = reinterpret_cast<T*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

		return t
			? t->HandleMessage(msg, wp, lp)
			: DefWindowProcW(hwnd, msg, wp, lp);
	}

	BaseWindow() : handle(NULL) {}

	BOOL Create(
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
		wc.lpfnWndProc = T::WindowProc;
		wc.hInstance = GetModuleHandleW(NULL);
		wc.lpszClassName = ClassName();

		RegisterClassW(&wc);

		handle = CreateWindowExW(
			exstyle, ClassName(), winname, style,
			x, y, width, height,
			parent, menu, wc.hInstance, this);

		return handle ? TRUE : FALSE;
	}

	HWND Window() const
	{
		return handle;
	}
};


class MainWindow : public BaseWindow<MainWindow>
{
public:
	wchar_t const* ClassName() const override
	{
		return L"MainWindow";
	}

	LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam) override
	{
		PAINTSTRUCT ps = { 0 };
		HDC hdc = NULL;
		switch (msg)
		{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_PAINT:
			hdc = BeginPaint(handle, &ps);
			FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
			EndPaint(handle, &ps);
			return 0;
		}
		return DefWindowProcW(handle, msg, wparam, lparam);
	}
};


int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int cmdshow)
{
	MainWindow mw;
	assert(mw.Create(
		L"你好 おはようございます مرحبًا 여보세요",
		WS_OVERLAPPEDWINDOW));

	ShowWindow(mw.Window(), cmdshow);

	// Run message loop
	MSG msg = { 0 };
	while (GetMessageW(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return 0;
}

