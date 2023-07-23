#include "flutter_sarec.h"

#include <mmdeviceapi.h>
#include <audioclient.h>
#include <fstream>
#include <thread>
#include <memory>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "winmm.lib")

#define S2IMPL(sareClient) (reinterpret_cast<SarectClientWindowImpl*>(sareClient))


struct SarectClientWindowImpl{
	IAudioCaptureClient* pCaptureClient = nullptr;
	WAVEFORMATEX* pWaveFormatEx = nullptr;
	IMMDeviceEnumerator* pEnumerator = nullptr;
	IMMDevice* pDevice = nullptr;
	IAudioClient* pAudioClient = nullptr;
	HANDLE hAudioFile = nullptr;
	std::thread runner;
	bool isRecording=false;
	bool pauseRecording=false;
};



FFI_PLUGIN_EXPORT void* CreateSarecClient(){
	CoInitialize(nullptr);
	IAudioCaptureClient* pCaptureClient = nullptr;
	WAVEFORMATEX* pWaveFormatEx = nullptr;
	IMMDeviceEnumerator* pEnumerator = nullptr;
	IMMDevice* pDevice = nullptr;
	IAudioClient* pAudioClient = nullptr;
	HANDLE hAudioFile = nullptr;
	
	HRESULT hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void**)&pEnumerator
    );

    if (FAILED(hr))
    {
        CoUninitialize();
        return nullptr;
    }
	
	hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr))
    {
        return nullptr;
    }

    // Initialize the audio client
    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pAudioClient);
    if (FAILED(hr))
    {
        pDevice->Release();
        return nullptr;
    }

    // Get the mix format of the audio stream
    hr = pAudioClient->GetMixFormat(&pWaveFormatEx);
    if (FAILED(hr))
    {
        pAudioClient->Release();
        pDevice->Release();
        return nullptr;
    }
    
    // Initialize the audio client in shared mode with stream flags to loopback capture
    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, pWaveFormatEx, nullptr);
    if (FAILED(hr))
    {
        CoTaskMemFree(pWaveFormatEx);
        pAudioClient->Release();
        pDevice->Release();
        return nullptr;
    }

    // Get the audio capture client
    hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);
    if (FAILED(hr))
    {
        pAudioClient->Release();
        pDevice->Release();
        return nullptr;
    }
	
	
	SarectClientWindowImpl* sareClient=new SarectClientWindowImpl();
	
	sareClient->pEnumerator=pEnumerator;
	sareClient->pCaptureClient=pCaptureClient;
	sareClient->pWaveFormatEx=pWaveFormatEx;
	sareClient->pDevice=pDevice;
	sareClient->pAudioClient=pAudioClient;
	sareClient->hAudioFile=hAudioFile;
	
	return sareClient;
}

FFI_PLUGIN_EXPORT intptr_t Start(void* client,const char* filename){
	if (client==nullptr) return 0;
	if (S2IMPL(client)->isRecording) return 0;
	auto sareClient=S2IMPL(client);
	
	size_t size;
	mbstowcs_s(&size, NULL, 0, filename, _TRUNCATE);  // Get the size of the destination string
	wchar_t* destination = new wchar_t[size];  // Allocate memory for the destination string

	mbstowcs_s(NULL, destination, size, filename, _TRUNCATE);  // Pe
	
	HANDLE hAudioFile = CreateFile(destination, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	
	if (hAudioFile == INVALID_HANDLE_VALUE)
    {
		delete [] destination;
        return 0;
    }
	sareClient->hAudioFile=hAudioFile;
	
	sareClient->isRecording=true;
	
	sareClient->pAudioClient->Start();
	
	sareClient->runner=std::thread([destination](SarectClientWindowImpl* cl){
		HRESULT hr;
		BYTE* pData;
		UINT32 numFramesAvailable;
		DWORD flags;
		while (cl->isRecording)
		{
			if (cl->pauseRecording) continue;
			
			hr = cl->pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);
			if (FAILED(hr))
			{
				break;
			}
			// Calculate the data size in bytes
			DWORD dataSizeBytes = numFramesAvailable * cl->pWaveFormatEx->nBlockAlign;
			// Write audio data to the file
			DWORD dwNumBytesWritten;
			WriteFile(cl->hAudioFile, pData, dataSizeBytes, &dwNumBytesWritten, nullptr);
			cl->pCaptureClient->ReleaseBuffer(numFramesAvailable);
			if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
			{
				// Audio stream is silent, do something if needed.
			}
		}
		
		delete [] destination;
	},sareClient);
	sareClient->runner.detach();
	return 1;
}

FFI_PLUGIN_EXPORT intptr_t Pause(void* client){
	if (client==nullptr) return 0;
	auto sareClient=S2IMPL(client);
	sareClient->pauseRecording=true;
	return 1;
	
}

FFI_PLUGIN_EXPORT intptr_t Resume(void* client){
	if (client==nullptr) return 0;
	auto sareClient=S2IMPL(client);
	sareClient->pauseRecording=false;
	return 1;
}

FFI_PLUGIN_EXPORT intptr_t Stop(void* client){
	if (client==nullptr) return 0;
	auto sareClient=S2IMPL(client);
	sareClient->isRecording=false;
	sareClient->pauseRecording=false;
	sareClient->pAudioClient->Stop();
    CloseHandle(sareClient->hAudioFile);
	return 1;
}

FFI_PLUGIN_EXPORT intptr_t IsRecording(void* client){
	if (client==nullptr) return 0;
	auto sareClient=S2IMPL(client);
	return sareClient->isRecording=true;
}


FFI_PLUGIN_EXPORT intptr_t SaveToWav(void*,const char*){
	
	return 1;
}

FFI_PLUGIN_EXPORT intptr_t DestroySarecClient(void* client){
	if (client==nullptr) return 0;
	
	auto sareClient=S2IMPL(client);
	sareClient->pCaptureClient->Release();
    sareClient->pAudioClient->Release();
    sareClient->pDevice->Release();
    CoTaskMemFree(sareClient->pWaveFormatEx);
	sareClient->pEnumerator->Release();
	delete [] sareClient;
	CoUninitialize();
	return 1;
}