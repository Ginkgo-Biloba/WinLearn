#pragma once


static void popup_error(WCHAR const* info = L"")
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


inline void PopUpError(WCHAR const* info) { popup_error(info); }

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

	BOOL save(HDC hdc, CStringW name);

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



// from OpenCV window_w32.cpp
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


static BOOL SaveBitmap(CStringW name, BITMAP const& src)
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
	HANDLE fid = CreateFileW(name.GetString(), GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!fid)
		return FALSE;
	WriteFile(fid, &file, sizeof(file), &written, NULL);
	WriteFile(fid, &info, sizeof(info), &written, NULL);
	WriteFile(fid, src.bmBits, szImg, &written, NULL);
	CloseHandle(fid);
	return TRUE;
}


template <typename T>
BOOL BaseWindow<T>::save(HDC dcwnd, CStringW name)
{
	HDC dc = NULL;
	HBITMAP objBmp = NULL;
	HGDIOBJ objOld = NULL;
	BITMAP bmp;
	BITMAPFILEHEADER fileHdr = { 0 };
	BOOL ret = TRUE;

	do
	{
		dc = CreateCompatibleDC(dcwnd);
		if (!dc)
		{
			PopUpError(L"CreateCompatibleDC 失败，不能创建中间上下文");
			break;
		}

		RECT rc;
		GetClientRect(wnd, &rc);
		POINT sz = { rc.right - rc.left, rc.bottom - rc.top };
		BITMAPINFO infoHdr = { 0 };
		FillBitmapInfo(&infoHdr, sz.x, sz.y, 24, 1);
		objBmp = CreateDIBSection(dc, &infoHdr, DIB_RGB_COLORS, NULL, 0, 0);
		if (!objBmp)
		{
			PopUpError(L"CreateCompatibleBitmap 失败");
			break;
		}

		// Bit block transfer into our compatible memory DC
		objOld = SelectObject(dc, objBmp);
		ret = BitBlt(dc, 0, 0, sz.x, sz.y, dcwnd, 0, 0, SRCCOPY);
		if (!ret)
		{
			PopUpError(L"BitBlt 失败，不能复制截图到当前位图");
			break;
		}
		ret = GetObjectW(objBmp, sizeof(bmp), &bmp);
		if (!ret)
		{
			PopUpError(L"不能获取当前位图信息");
			break;
		}

		SaveBitmap(name, bmp);
	} while (0, 0);

	if (objOld)
		SelectObject(dc, objOld);
	if (dc)
		DeleteDC(dc);
	if (objBmp)
		DeleteObject(objBmp);
	return ret;
}

