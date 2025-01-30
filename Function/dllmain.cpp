﻿#include "pch.h"
#include <string>
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <fstream>
#include <avrt.h>
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
#define CHECK_HR(hr) if (FAILED(hr)) { wstring errMsg = L"Error: " + to_wstring(hr); MessageBox(NULL, errMsg.c_str(), L"Error", MB_ICONERROR); return hr; }

// 播放音频文件
HRESULT PlayAudioFileWithVolume(const wstring& filePath, float volume) {
    // 检查音频文件是否存在
    ifstream file(filePath);
    if (!file.good()) {
        wstring errMsg = L"File not found: " + filePath;
        MessageBox(NULL, errMsg.c_str(), L"Error", MB_ICONERROR);
        return E_FAIL;
    }

    // 检查音量范围是否在合理范围内 (0.0 到 1.0)
    if (volume < 0.0f || volume > 1.0f) {
        wstring errMsg = L"Volume out of range: " + to_wstring(volume);
        MessageBox(NULL, errMsg.c_str(), L"Error", MB_ICONERROR);
        return E_FAIL;
    }
    
    HRESULT hr = S_OK;
    IMMDeviceEnumerator* deviceEnumerator = nullptr;
    IMMDevice* audioDevice = nullptr;
    IAudioClient* audioClient = nullptr;
    IAudioRenderClient* renderClient = nullptr;
    WAVEFORMATEX* waveFormat = nullptr;
    UINT32 bufferFrameCount;
    BYTE* audioData;
    DWORD flags = 0;
    HANDLE hEvent = NULL;
    DWORD taskIndex = 0;

    // 初始化COM库
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    CHECK_HR(hr);

    // 创建设备枚举器
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&deviceEnumerator);
    CHECK_HR(hr);

    // 获取默认音频输出设备
    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &audioDevice);
    CHECK_HR(hr);

    // 激活音频客户端
    hr = audioDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&audioClient);
    CHECK_HR(hr);

    // 获取混合格式
    hr = audioClient->GetMixFormat(&waveFormat);
    CHECK_HR(hr);

    // 初始化音频客户端（独占模式）
    hr = audioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 10000000, 0, waveFormat, NULL);
    CHECK_HR(hr);

    // 创建事件句柄
    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!hEvent) {
        wstring errMsg = L"CreateEvent failed: " + to_wstring(GetLastError());
        MessageBox(NULL, errMsg.c_str(), L"Error", MB_ICONERROR);
        return E_FAIL;
    }

    // 设置事件句柄
    hr = audioClient->SetEventHandle(hEvent);
    CHECK_HR(hr);

    // 获取缓冲区大小
    hr = audioClient->GetBufferSize(&bufferFrameCount);
    CHECK_HR(hr);

    // 获取音频渲染客户端
    hr = audioClient->GetService(__uuidof(IAudioRenderClient), (void**)&renderClient);
    CHECK_HR(hr);

    audioData = new BYTE[bufferFrameCount * waveFormat->nBlockAlign];

    // 在此处将音频文件数据加载到audioData缓冲区中
    // 此示例假设audioData已经填充了有效的音频样本

    hr = renderClient->GetBuffer(bufferFrameCount, &audioData);
    CHECK_HR(hr);

    // 将音频样本复制到audioData缓冲区中
    // 请确保遵守音频客户端的波形格式

    hr = renderClient->ReleaseBuffer(bufferFrameCount, flags);
    CHECK_HR(hr);

    // 启动音频客户端
    hr = audioClient->Start();
    CHECK_HR(hr);

    // 设置音量
    ISimpleAudioVolume* simpleAudioVolume = nullptr;
    hr = audioClient->GetService(__uuidof(ISimpleAudioVolume), (void**)&simpleAudioVolume);
    CHECK_HR(hr);

    hr = simpleAudioVolume->SetMasterVolume(volume, NULL);
    CHECK_HR(hr);

    // 等待音频播放完成
    WaitForSingleObject(hEvent, INFINITE);

    // 停止音频客户端并释放资源
    audioClient->Stop();
    audioClient->Release();
    renderClient->Release();
    deviceEnumerator->Release();
    CoUninitialize();
    delete[] audioData;

    return S_OK;
}

//EXPORT HRESULT PlayAudioFileWithVolume
EXPORT_DLL int PlaySound(const float volume, const wstring filePath) {
    HRESULT hr = PlayAudioFileWithVolume(filePath, volume);
    if (FAILED(hr)) {
        wstring errMsg = L"Playback failed!";
        MessageBox(NULL, errMsg.c_str(), L"Error", MB_ICONERROR);
        return -1;
    }

    return 0;
}
