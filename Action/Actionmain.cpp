#include <iostream>
#include <Windows.h>
#include <gdiplus.h>
#include <gdiplusgraphics.h>
#include <tchar.h>
#include <string>
#include <io.h>
#include <fcntl.h>
using namespace std;

wstring removeLeadingDash(const wstring& param)
{
    return (param[0] == L'-') ? param.substr(1) : param;
}

int PlayAudio(const float volume, const wstring type, const HMODULE hModule) {
	wstring filePath = L"";
    if(type == L"long")
	{
		filePath = L".\\ClassHelper\\Resource\\Ring(full).wav";
	}
	else if (type == L"standard")
	{
		filePath = L".\\ClassHelper\\Resource\\Ring(standard).wav";
	}
    else if (type == L"short")
    {
        filePath = L".\\ClassHelper\\Resource\\Ring(short).wav";
    }
    else if (type == L"single")
    {
        filePath = L".\\ClassHelper\\Resource\\Ring(single).wav";
    }
	else
	{
		MessageBox(NULL, (L"Error: 0x80070057 " + wstring(type.begin(), type.end())).c_str(), L"Error", MB_OK);
		return -1;
	}
    typedef int(*PlayAudio)(const float volume, const wstring filePath);
	PlayAudio playAudio = (PlayAudio)GetProcAddress(hModule, "PlayAudio");
	if (playAudio == NULL) {
		cout << "GetProcAddress failed: " << GetLastError() << endl;
		return -1;
	}
	playAudio(volume, filePath);
	return 0;
}

int PlayAudio(const HMODULE hModule) {

    typedef int(*ShutDown)();
	ShutDown shutdown = (ShutDown)GetProcAddress(hModule, "ShutdownComputer");
	if (shutdown == NULL) {
		cout << "GetProcAddress failed: " << GetLastError() << endl;
		return -1;
	}
	shutdown();
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    // 获取并解析命令行参数
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (argc < 4)
    {
        MessageBox(NULL, L"Error: 0x80070057", L"Error", MB_OK);
        return 1;
    }

    // 获取并去掉每个参数的前导 '-'
    wstring param1 = removeLeadingDash(argv[1]);
    wstring param2 = removeLeadingDash(argv[2]);
    wstring param3_str = removeLeadingDash(argv[3]);

    // 尝试将第三个参数转换为 float
    float param3;
    try
    {
        param3 = stof(param3_str);
    }
    catch (const exception& e)
    {
        MessageBox(NULL, L"Error: 0x8000FFFF Convent volume to float!", L"Error", MB_OK);
        return -1;
    }

    HMODULE hModule = LoadLibrary(L".\\ClassHelper\\Core.dll");
    if (hModule == NULL) {
        cout << "LoadLibrary failed: " << GetLastError() << endl;
        return -1;
    }

    if (param1 == L"ring")
    {
        PlayAudio(param3, param2, hModule);
    }
	if (param1 == L"sd")
	{
		
	}
    else
    {
        MessageBox(NULL, (L"Error: 0x80070057 " + wstring(param1.begin(), param1.end())).c_str(), L"Error", MB_OK);
        return 1;
    }

    FreeLibrary(hModule);
    return 0;
}