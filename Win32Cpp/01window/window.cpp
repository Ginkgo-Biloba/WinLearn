#include <Windows.h>

LRESULT CALLBACK WinMainProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_CLOSE:
	{
		if (MessageBoxW(hwnd, L"真的要关？", L"确认？", MB_OKCANCEL) == IDOK)
			DestroyWindow(hwnd);
		return 0;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
		EndPaint(hwnd, &ps);
		return 0;
	}
	}

	return DefWindowProcW(hwnd, msg, wp, lp);
}


int WINAPI wWinMain(HINSTANCE hins, HINSTANCE, PWSTR cmd, int show)
{
	(void)(cmd);
	wchar_t const* WinMainName = L"WinMainClass";

	WNDCLASSW wc = {0};
	wc.lpfnWndProc = WinMainProc;
	wc.hInstance = hins;
	wc.lpszClassName = WinMainName;
	RegisterClassW(&wc);

	HWND hwnd = CreateWindowExW(
		0,
		WinMainName,
		L"你好 おはようございます مرحبًا 여보세요",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
		NULL,
		NULL,
		hins,
		NULL);

	if (!hwnd)
		return -1;

	ShowWindow(hwnd, show);

	MSG msg = {};
	while (GetMessageW(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return 0;
}

