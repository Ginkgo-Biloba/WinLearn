// 00unicode.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#define WIN32_LEAN_AND_MEDIUM
#include <Windows.h>
#include <atlstr.h>

int printf_utf16(wchar_t const* fmt, ...)
{
	DWORD writen = 0;
	va_list arg;
	va_start(arg, fmt);
#if 1
	::CStringW cs;
	cs.FormatV(fmt, arg);
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
		cs.GetString(), cs.GetLength(), &writen, NULL);
#else 
	wchar_t ws[1024];
	writen = _vsnwprintf_s(ws, 1024, fmt, arg);
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
		ws, writen, &writen, NULL);
#endif
	va_end(arg);
	return static_cast<int>(writen);
}


static wchar_t const* slang[] =
{
	L"你好",
	L"ཁམས་བཟང་།",
	L"こんにちは",
	L"안녕하세요",
};


int wmain(int, wchar_t** argv)
{
	printf_utf16(L"参数 1 是 %s\n", argv[1]);
	::CStringW cs;
	for (size_t i = sizeof(slang) / sizeof(slang[0]); i--;)
	{
		cs.Append(slang[i]);
		cs.AppendChar(L'\n');
	}
	printf_utf16(L"\n😉\n%s", cs.GetString());
	return 0;
}

