#define WIN32_LEAN_AND_MEAN
#include <atlbase.h>
#include <ShObjIdl.h>
#include <type_traits>
#undef NDEBUG
#include <cassert>

#define CONCAT_EXPR(x, y) x##y
#define CONCAT(x, y) CONCAT_EXPR(x, y)


struct CoGaurd
{
	~CoGaurd() { CoUninitialize(); }
};


int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	HRESULT ret = CoInitializeEx(NULL,
		COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(ret))
		return ret;
	CoGaurd cogaurd;

	CComPtr<IFileOpenDialog> ifp;
	ret = ifp.CoCreateInstance(__uuidof(FileOpenDialog));
	if (FAILED(ret))
		return ret;
	
	ret = ifp->Show(NULL);
	if (FAILED(ret))
		return ret;
	
	CComPtr<IShellItem> item;
	ret = ifp->GetResult(&item);
	if (FAILED(ret))
		return ret;

	wchar_t* filename = NULL;
	ret = item->GetDisplayName(SIGDN_FILESYSPATH, &filename);
	if (FAILED(ret))
		return ret;

	MessageBoxW(NULL, filename, L"ƒ„—°‘Ò¡À", MB_OK);
	CoTaskMemFree(filename);

	return 0;
}

