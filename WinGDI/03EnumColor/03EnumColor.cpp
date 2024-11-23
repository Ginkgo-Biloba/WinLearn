#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windowsx.h>
#include <atlstr.h>
#include <algorithm>
#include <cassert>


static int CALLBACK MyEnumProc(LPVOID vp, LPARAM lp)
{
	LPLOGPEN pen = reinterpret_cast<LPLOGPEN>(vp);
	COLORREF* color = reinterpret_cast<COLORREF*>(lp);
	int idx = 0;
	if (pen->lopnStyle == PS_SOLID)
	{
		idx = color[0];
		if (idx <= 0)
			return 0;
		color[idx] = pen->lopnColor;
		color[0] -= 1;
	}
	return 1;
}


////////////////////////////////////////////////////////////


int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR cmdline, int cmdshow)
{
	UNREFERENCED_PARAMETER(instance);
	UNREFERENCED_PARAMETER(cmdline);
	UNREFERENCED_PARAMETER(cmdshow);

	HDC dc = CreateCompatibleDC(NULL);
	HANDLE heap = GetProcessHeap();
	int count = GetDeviceCaps(dc, NUMCOLORS);
	COLORREF* color = reinterpret_cast<COLORREF*>(
		HeapAlloc(heap, 0, sizeof(color[0]) * (count + 1)));
	color[0] = count;
	EnumObjects(dc, OBJ_PEN, MyEnumProc, reinterpret_cast<LPARAM>(color));
	color[0] = count;

	CStringW E;
	E.Format(L"%d colors\n", count);
	OutputDebugStringW(E.GetString());
	HeapFree(heap, 0, color);
	DeleteDC(dc);

	return count;
}