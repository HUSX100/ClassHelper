#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <string>
#include <thread>
#include <atomic>

// 音量控制接口
IAudioEndpointVolume* pVolumeControl = nullptr;
std::atomic<bool> playing{ false };  // 标记音频是否正在播放

// 设置音量并取消静音
bool SetDeviceVolumeAndUnmute(float volume) {
    if (pVolumeControl) {
        // 设置音量
        HRESULT hr = pVolumeControl->SetMasterVolumeLevelScalar(volume, nullptr);
        if (FAILED(hr)) return false;

        // 取消静音
        hr = pVolumeControl->SetMute(FALSE, nullptr);  // FALSE 表示取消静音
        return SUCCEEDED(hr);
    }
    return false;
}

// 音量监控线程，确保音量不被静音
void MonitorVolume(float volume) {
    while (playing.load()) {
        float currentVolume = 0.0f;
        BOOL isMuted = FALSE;

        // 检查音量和静音状态
        HRESULT hr = pVolumeControl->GetMasterVolumeLevelScalar(&currentVolume);
        HRESULT hrMute = pVolumeControl->GetMute(&isMuted);

        if (SUCCEEDED(hr) && currentVolume < volume) {
            pVolumeControl->SetMasterVolumeLevelScalar(volume, nullptr);  // 恢复音量
        }

        if (SUCCEEDED(hrMute) && isMuted) {
            pVolumeControl->SetMute(FALSE, nullptr);  // 取消静音
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 每0.5秒检查一次
    }
}

// 初始化音量控制
bool InitializeVolumeControl() {
    CoInitialize(nullptr);

    // 获取音频设备的枚举器
    IMMDeviceEnumerator* pEnumerator = nullptr;
    HRESULT hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pEnumerator));
    if (FAILED(hr)) {
        MessageBox(nullptr, L"无法创建设备枚举器！", L"错误", MB_ICONERROR);
        return false;
    }

    // 获取默认音频设备
    IMMDevice* pDevice = nullptr;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"无法获取默认音频设备！", L"错误", MB_ICONERROR);
        pEnumerator->Release();
        return false;
    }

    // 获取音量控制接口
    hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr, (void**)&pVolumeControl);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"无法激活音量控制接口！", L"错误", MB_ICONERROR);
        pDevice->Release();
        pEnumerator->Release();
        return false;
    }

    pDevice->Release();
    pEnumerator->Release();
    return true;
}

// 释放音量控制接口
void ReleaseVolumeControl() {
    if (pVolumeControl) {
        pVolumeControl->Release();
        pVolumeControl = nullptr;
    }
    CoUninitialize();
}

// 播放音频文件（隐藏cmd窗口）
void PlayAudio(const std::string& filePath) {
    playing.store(true);

    // 将命令转换为宽字符字符串
    std::wstring command = L"ffplay -autoexit -nodisp \"" + std::wstring(filePath.begin(), filePath.end()) + L"\"";

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;  // 隐藏窗口

    if (CreateProcessW(nullptr, const_cast<LPWSTR>(command.c_str()), nullptr, nullptr, FALSE,
        0, nullptr, nullptr, &si, &pi)) {
        // 等待音频播放完成
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else {
        MessageBox(nullptr, L"无法启动 ffplay！", L"错误", MB_ICONERROR);
    }
    playing.store(false);  // 播放结束
}

// 主窗口入口函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    float volume = 1.0f;  // 设置音量为100%
    std::string audioFilePath = "Ring(Full).wav";  // 音频文件路径

    if (!InitializeVolumeControl()) {
        return -1;
    }

    // 设置音量并取消静音
    if (!SetDeviceVolumeAndUnmute(volume)) {
        MessageBox(nullptr, L"无法设置音量或取消静音！", L"错误", MB_ICONERROR);
        ReleaseVolumeControl();
        return -1;
    }

    // 启动音量监控线程
    std::thread volumeMonitor(MonitorVolume, volume);

    // 播放音频
    PlayAudio(audioFilePath);

    // 停止音量监控
    playing.store(false);
    volumeMonitor.join();

    ReleaseVolumeControl();

    return 0;
}
