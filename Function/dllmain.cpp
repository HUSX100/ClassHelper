#include "pch.h"

#pragma comment(lib, "gdiplus.lib")


using namespace Gdiplus;
using namespace std;


// DllMain
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

// 检查HRESULT是否成功
#define CHECK_HR(hr) \
    if (FAILED(hr)) { \
        std::wstringstream ss; \
        ss << L"Error: 0x" << std::hex << std::setw(8) << std::setfill(L'0') << hr; \
        std::wstring errMsg = ss.str(); \
        MessageBox(NULL, errMsg.c_str(), L"Error", MB_ICONERROR); \
        return hr; \
    }

// 检查文件路径是否存在
HRESULT CheckPath(const wstring& filePath) {
    ifstream file(filePath);
    if (!file.good()) {
        wstring errMsg = L"File not found: " + filePath;
        MessageBox(NULL, errMsg.c_str(), L"Error", MB_ICONERROR);
        return E_FAIL;
    }
    else return S_OK;
}

// 播放音频文件
IAudioEndpointVolume* pVolumeControl = nullptr;
atomic<bool> playing{ false };  // 标记音频是否正在播放
HRESULT PlayAudioFileWithVolume(const wstring& audioFilePath, float volume) {
	CheckPath(audioFilePath);

    HRESULT hr = CoInitialize(nullptr);
    CHECK_HR(hr);

    IMMDeviceEnumerator* pEnumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pEnumerator));
    CHECK_HR(hr);

    IMMDevice* pDevice = nullptr;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    CHECK_HR(hr);

    hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr, (void**)&pVolumeControl);
    CHECK_HR(hr);

    pDevice->Release();
    pEnumerator->Release();

    hr = pVolumeControl->SetMasterVolumeLevelScalar(volume, nullptr);
    CHECK_HR(hr);

    hr = pVolumeControl->SetMute(FALSE, nullptr);
    CHECK_HR(hr);

    playing.store(true);
    thread volumeMonitor([&]() {
        while (playing.load()) {
            float currentVolume = 0.0f;
            BOOL isMuted = FALSE;

            HRESULT hrVolume = pVolumeControl->GetMasterVolumeLevelScalar(&currentVolume);
            HRESULT hrMute = pVolumeControl->GetMute(&isMuted);

            if (SUCCEEDED(hrVolume) && currentVolume < volume) {
                pVolumeControl->SetMasterVolumeLevelScalar(volume, nullptr);  // 恢复音量
            }

            if (SUCCEEDED(hrMute) && isMuted) {
                pVolumeControl->SetMute(FALSE, nullptr);  // 取消静音
            }

            this_thread::sleep_for(chrono::milliseconds(100));  // 每0.1秒检查一次
        }
        });

    wstring command = L"ffplay -autoexit -nodisp \"" + audioFilePath + L"\"";
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if (CreateProcessW(nullptr, const_cast<LPWSTR>(command.c_str()), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else {
        playing.store(false);
        volumeMonitor.join();
        pVolumeControl->Release();
        pVolumeControl = nullptr;
        CoUninitialize();
        return E_FAIL;
    }

    playing.store(false);
    volumeMonitor.join();
    pVolumeControl->Release();
    pVolumeControl = nullptr;
    CoUninitialize();
    return S_OK;
}

// 显示图片函数
//Gdiplus::GdiplusStartupInput gdiplusStartupInput;
//ULONG_PTR gdiplusToken;
//GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
//Gdiplus::GdiplusShutdown(gdiplusToken);
HRESULT ShowImageWithParameter(const wstring& imagePath, int width, int height, int duration) {
    

    if (CheckPath(imagePath) != S_OK) {
        return E_FAIL;
    }

    HINSTANCE hInstance = GetModuleHandle(NULL);
    HWND hwnd = CreateWindowEx(0, L"STATIC", NULL, WS_POPUP | WS_VISIBLE,
        (GetSystemMetrics(SM_CXSCREEN) - width) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - height) / 2,
        width, height, NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        wstring errMsg = L"Failed to create window";
        MessageBox(NULL, errMsg.c_str(), L"Error", MB_ICONERROR);
        return E_FAIL;
    }

    HDC hdc = GetDC(hwnd);
    if (!hdc) {
        wstring errMsg = L"Failed to get device context";
        MessageBox(NULL, errMsg.c_str(), L"Error", MB_ICONERROR);
        DestroyWindow(hwnd);
        return E_FAIL;
    }

    // 这里使用智能指针管理Image对象
    unique_ptr<Image> pImage = nullptr;
    try {
        pImage = make_unique<Image>(imagePath.c_str());
        Status status = pImage->GetLastStatus();
        if (status != Ok) {
            wstring errMsg = L"Failed to load image: " + imagePath + L" Status: " + to_wstring(status);
            MessageBox(NULL, errMsg.c_str(), L"Error", MB_ICONERROR);
            throw runtime_error("Image load failed");
        }
    }
    catch (const exception& e) {
        MessageBox(NULL, L"Failed to create image", L"Error", MB_ICONERROR);
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        return E_FAIL;
    }

    Gdiplus::Graphics graphics(hdc);  // 使用GDI+ Graphics对象

    // 等待指定时间之前不要销毁HDC或Graphics对象
    graphics.DrawImage(pImage.get(), 0, 0, width, height);

    // 等待指定时间
    this_thread::sleep_for(chrono::seconds(duration));

    // 释放资源
    ReleaseDC(hwnd, hdc);   // 释放HDC
    DestroyWindow(hwnd);    // 销毁窗口
    return S_OK;
}

//终止进程函数
HRESULT TerminateProcessByName(const wstring& processName) {
    // 获取系统进程快照
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        MessageBox(NULL, L"Unable to create process snapshot", L"Error", MB_ICONERROR);
        return E_FAIL;
    }

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    // 获取第一个进程的信息
    if (!Process32First(hSnapshot, &processEntry)) {
        CloseHandle(hSnapshot);
        MessageBox(NULL, L"Unable to retrieve process information", L"Error", MB_ICONERROR);
        return E_FAIL;
    }

    do {
        // 检查进程名是否匹配
        if (processName == processEntry.szExeFile) {
            // 找到目标进程，尝试终止它
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processEntry.th32ProcessID);
            if (hProcess == NULL) {
                CloseHandle(hSnapshot);
                MessageBox(NULL, L"Unable to open the process", L"Error", MB_ICONERROR);
                return E_FAIL;
            }

            if (!TerminateProcess(hProcess, 0)) {
                CloseHandle(hProcess);
                CloseHandle(hSnapshot);
                MessageBox(NULL, L"Unable to terminate the process", L"Error", MB_ICONERROR);
                return E_FAIL;
            }

            CloseHandle(hProcess);
            CloseHandle(hSnapshot);
            //MessageBox(NULL, L"Process terminated successfully", L"Success", MB_ICONINFORMATION);
            return S_OK;
        }
    } while (Process32Next(hSnapshot, &processEntry));

    CloseHandle(hSnapshot);
    MessageBox(NULL, L"Process not found", L"Error", MB_ICONERROR);
    return E_FAIL;
}

//关机函数
HRESULT ShutdownComputerImmediately() {

    // 先广播关机请求
    if (!SendMessageTimeout(HWND_BROADCAST, WM_QUERYENDSESSION, 0, 0, SMTO_ABORTIFHUNG, 10000, NULL)) {
        MessageBox(NULL, L"Failed to broadcast shutdown request", L"Error", MB_ICONERROR);
        return E_FAIL;
    }

    // 发出关机请求
    if (!ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER)) {
        MessageBox(NULL, L"Unable to shutdown the computer", L"Error", MB_ICONERROR);
        return E_FAIL;
    }

    // 返回成功
    MessageBox(NULL, L"Shutdown initiated", L"Success", MB_ICONINFORMATION);
    return S_OK;
}

//EXPORT HRESULT PlayAudioFileWithVolume
EXPORT_DLL int PlayAudio(const float volume, const wstring filePath) {
    HRESULT hr = PlayAudioFileWithVolume(filePath, volume);
    if (FAILED(hr)) {
        wstring errMsg = L"Playback failed!";
        MessageBox(NULL, errMsg.c_str(), L"Error", MB_ICONERROR);
        return -1;
    }

    return 0;
}

//EXPORT HRESULT ShowImageWithParameter
EXPORT_DLL int ShowImage(const wstring& imagePath, int width, int height, int duration) {
    HRESULT hr = ShowImageWithParameter(imagePath,width,height,duration);
    if (FAILED(hr)) {
        wstring errMsg = L"ShowImage failed!";
        MessageBox(NULL, errMsg.c_str(), L"Error", MB_ICONERROR);
        return -1;
    }

    return 0;
}

//EXPORT HRESULT TerminateProcessByName
EXPORT_DLL int TerminateSingleProcess(const wstring& processName) {
	HRESULT hr = TerminateProcessByName(processName);
	if (FAILED(hr)) {
		wstring errMsg = L"TerminateProcess failed!";
		MessageBox(NULL, errMsg.c_str(), L"Error", MB_ICONERROR);
		return -1;
	}
	return 0;
}

//EXPORT HRESULT ShutdownComputerImmediately
EXPORT_DLL int ShutdownComputer() {
	HRESULT hr = ShutdownComputerImmediately();
	if (FAILED(hr)) {
		wstring errMsg = L"ShutdownComputer failed!";
		MessageBox(NULL, errMsg.c_str(), L"Error", MB_ICONERROR);
		return -1;
	}
	return 0;
}