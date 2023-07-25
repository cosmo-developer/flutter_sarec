#include "flutter_sarec.h"

#include <mmdeviceapi.h>
#include <audioclient.h>
#include <mmsystem.h>
#include <fstream>
#include <thread>
#include <memory>
#include <endpointvolume.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "winmm.lib")

#define S2IMPL(sareClient) (reinterpret_cast<SarectClientWindowImpl *>(sareClient))

#define MIC_NUM_BUFFERS 3
#define MIC_BUFFER_SIZE 4096
struct SarectClientWindowImpl
{
	IAudioCaptureClient *pCaptureClient = nullptr;
	WAVEFORMATEX *pWaveFormatEx = nullptr;
	IMMDeviceEnumerator *pEnumerator = nullptr;
	IMMDevice *pDevice = nullptr;
	IAudioClient *pAudioClient = nullptr;
	HANDLE hAudioFile = nullptr;
	std::thread runner;
	bool isRecording = false;
	bool pauseRecording = false;
	// For mic
	WAVEFORMATEX MicWaveFormat;
	HWAVEIN hWaveIn;
	WAVEHDR waveHeaders[MIC_NUM_BUFFERS];
	HANDLE hMicAudioFile;
	bool muteMic = false;
};

void CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	if (uMsg == WIM_DATA)
	{
		WAVEHDR *pWaveHdr = (WAVEHDR *)dwParam1;
		SarectClientWindowImpl *sareClient = reinterpret_cast<SarectClientWindowImpl *>(dwInstance);
		// Write audio data to the file
		// outFile.write((char*)pWaveHdr->lpData, pWaveHdr->dwBytesRecorded);
		if (!sareClient->pauseRecording){
			DWORD dwNumBytesWritten;
			WriteFile(sareClient->hMicAudioFile, pWaveHdr->lpData, pWaveHdr->dwBytesRecorded, &dwNumBytesWritten, nullptr);
		}
		// Reuse the buffer
		waveInAddBuffer(sareClient->hWaveIn, pWaveHdr, sizeof(WAVEHDR));
	}
}



FFI_PLUGIN_EXPORT void *CreateSarecClient()
{
	CoInitialize(nullptr);
	IAudioCaptureClient *pCaptureClient = nullptr;
	WAVEFORMATEX *pWaveFormatEx = nullptr;
	IMMDeviceEnumerator *pEnumerator = nullptr;
	IMMDevice *pDevice = nullptr;
	IAudioClient *pAudioClient = nullptr;
	HANDLE hAudioFile = nullptr;

	HRESULT hr = CoCreateInstance(
		__uuidof(MMDeviceEnumerator),
		nullptr,
		CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		(void **)&pEnumerator);

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
	hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void **)&pAudioClient);
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
	hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void **)&pCaptureClient);
	if (FAILED(hr))
	{
		pAudioClient->Release();
		pDevice->Release();
		return nullptr;
	}

	SarectClientWindowImpl *sareClient = new SarectClientWindowImpl;

	sareClient->pEnumerator = pEnumerator;
	sareClient->pCaptureClient = pCaptureClient;
	sareClient->pWaveFormatEx = pWaveFormatEx;
	sareClient->pDevice = pDevice;
	sareClient->pAudioClient = pAudioClient;
	sareClient->hAudioFile = hAudioFile;
	// sareClient->phWaveIn=phWaveIn;
	// sareClient->pMicWaveFormatEx;
	// sareClient->pwaveHeaders=pwaveHeaders;

	return sareClient;
}

FFI_PLUGIN_EXPORT intptr_t Start(void *client, const char *filename)
{
	if (client == nullptr)
		return 0;
	if (S2IMPL(client)->isRecording)
		return 0;
	auto sareClient = S2IMPL(client);

	size_t size;
	mbstowcs_s(&size, NULL, 0, filename, _TRUNCATE); // Get the size of the destination string
	wchar_t *destination = new wchar_t[size];		 // Allocate memory for the destination string

	std::string micFileName=std::string(filename)+std::string("_mic.raw");

	const char* micFileNameCstr=micFileName.c_str();

	size_t micFileNameSize;

	mbstowcs_s(&micFileNameSize,NULL,0,micFileNameCstr,_TRUNCATE);
	wchar_t* micDestination=new wchar_t[micFileNameSize];

	mbstowcs_s(NULL, destination, size, filename, _TRUNCATE); // Pe
	mbstowcs_s(NULL,micDestination,micFileNameSize,micFileNameCstr,_TRUNCATE);

	HANDLE hAudioFile = CreateFile(destination, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (hAudioFile == INVALID_HANDLE_VALUE)
	{
		delete[] destination;
		return 0;
	}

	HANDLE micAudioFile=CreateFile(micDestination,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    
	if (micAudioFile == INVALID_HANDLE_VALUE)
	{
		delete[] micDestination;
		return 0;
	}
	
	sareClient->hAudioFile = hAudioFile;
	sareClient->hMicAudioFile=micAudioFile;

	sareClient->isRecording = true;

	sareClient->pAudioClient->Start();

	sareClient->runner = std::thread([destination,micDestination](SarectClientWindowImpl *cl)
									 {
		//Mic Area Intialization and starting
		HRESULT hr;
		BYTE* pData;
		UINT32 numFramesAvailable;
		DWORD flags;
		WAVEFORMATEX* pMicWaveFormatEx=&cl->MicWaveFormat;
		pMicWaveFormatEx->wFormatTag=WAVE_FORMAT_PCM;
		pMicWaveFormatEx->nChannels=1;
		pMicWaveFormatEx->nSamplesPerSec=32000;
		pMicWaveFormatEx->wBitsPerSample=32;
		pMicWaveFormatEx->nBlockAlign=4;
		pMicWaveFormatEx->nAvgBytesPerSec=128000;
		pMicWaveFormatEx->cbSize=0;
		
		
		HWAVEIN* phWaveIn=&cl->hWaveIn;
		
		MMRESULT result=waveInOpen(phWaveIn,WAVE_MAPPER,pMicWaveFormatEx,(DWORD_PTR)waveInProc,(DWORD_PTR)cl,CALLBACK_FUNCTION);
		if (result!=MMSYSERR_NOERROR){
			CoTaskMemFree(pMicWaveFormatEx);
			return nullptr;
		}
		
		WAVEHDR* pwaveHeaders=cl->waveHeaders;
		
		for (int i=0;i < MIC_NUM_BUFFERS; i++){
			pwaveHeaders[i].lpData=new char[MIC_BUFFER_SIZE];
			pwaveHeaders[i].dwBufferLength=MIC_BUFFER_SIZE;
			pwaveHeaders[i].dwBytesRecorded=0;
			pwaveHeaders[i].dwUser=0;
			pwaveHeaders[i].dwFlags=0;
			pwaveHeaders[i].dwLoops=0;
			
			result=waveInPrepareHeader(*phWaveIn,&pwaveHeaders[i],sizeof(WAVEHDR));
			
			if (result!=MMSYSERR_NOERROR){
				CoTaskMemFree(pMicWaveFormatEx);
				delete[] destination;
				return nullptr;
			}
		}
		
		
		for (int i=0;i<MIC_NUM_BUFFERS;i++){
			result=waveInAddBuffer(*phWaveIn,&pwaveHeaders[i],sizeof(WAVEHDR));
			
			if (result != MMSYSERR_NOERROR) {
				CoTaskMemFree(pMicWaveFormatEx);
				delete [] destination;
				return nullptr;
			}
		}
		result = waveInStart(*phWaveIn);
		if (result != MMSYSERR_NOERROR) {
			delete [] destination;
			delete [] micDestination;
			return nullptr;
		}
		
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
		
		},
									 sareClient);
	sareClient->runner.detach();
	return 1;
}

FFI_PLUGIN_EXPORT intptr_t Pause(void *client)
{
	if (client == nullptr)
		return 0;
	auto sareClient = S2IMPL(client);
	sareClient->pauseRecording = true;
	return 1;
}

FFI_PLUGIN_EXPORT intptr_t Resume(void *client)
{
	if (client == nullptr)
		return 0;
	auto sareClient = S2IMPL(client);
	sareClient->pauseRecording = false;
	return 1;
}

FFI_PLUGIN_EXPORT intptr_t Stop(void *client)
{
	if (client == nullptr)
		return 0;
	auto sareClient = S2IMPL(client);
	sareClient->isRecording = false;
	sareClient->pauseRecording = false;
	sareClient->pAudioClient->Stop();
	waveInStop(sareClient->hWaveIn);
	// waveInReset(sareClient->hWaveIn);

	for (int i=0;i<MIC_NUM_BUFFERS;i++){
		waveInUnprepareHeader(sareClient->hWaveIn,&sareClient->waveHeaders[i],sizeof(WAVEHDR));
		free(sareClient->waveHeaders[i].lpData);
	}

	waveInClose(sareClient->hWaveIn);
	CloseHandle(sareClient->hMicAudioFile);
	CloseHandle(sareClient->hAudioFile);
	return 1;
}

FFI_PLUGIN_EXPORT intptr_t IsRecording(void *client)
{
	if (client == nullptr)
		return 0;
	auto sareClient = S2IMPL(client);
	return sareClient->isRecording = true;
}

FFI_PLUGIN_EXPORT intptr_t SaveToWav(void *, const char *)
{

	return 1;
}

FFI_PLUGIN_EXPORT intptr_t DestroySarecClient(void *client)
{
	if (client == nullptr)
		return 0;

	auto sareClient = S2IMPL(client);
	sareClient->pCaptureClient->Release();
	sareClient->pAudioClient->Release();
	sareClient->pDevice->Release();
	CoTaskMemFree(sareClient->pWaveFormatEx);
	sareClient->pEnumerator->Release();
	delete sareClient;
	CoUninitialize();
	return 1;
}

FFI_PLUGIN_EXPORT intptr_t GetSystemRecordingFormatTag(void *client)
{
	if (client == nullptr)
		return 0;
	auto sareClient = S2IMPL(client);
	return sareClient->pWaveFormatEx->wFormatTag;
}

FFI_PLUGIN_EXPORT intptr_t GetSystemRecordingChannels(void *client)
{
	if (client == nullptr)
		return 0;
	auto sareClient = S2IMPL(client);
	return sareClient->pWaveFormatEx->nChannels;
}

FFI_PLUGIN_EXPORT intptr_t GetSystemRecordingSamplesPerSec(void *client)
{
	if (client == nullptr)
		return 0;
	auto sareClient = S2IMPL(client);
	return sareClient->pWaveFormatEx->nSamplesPerSec;
}

FFI_PLUGIN_EXPORT intptr_t GetSystemRecordingBitsPerSample(void *client)
{
	if (client == nullptr)
		return 0;
	auto sareClient = S2IMPL(client);
	return sareClient->pWaveFormatEx->wBitsPerSample;
}

FFI_PLUGIN_EXPORT intptr_t GetSystemRecordingBlockAlign(void *client)
{
	if (client == nullptr)
		return 0;
	auto sareClient = S2IMPL(client);
	return sareClient->pWaveFormatEx->nBlockAlign;
}

FFI_PLUGIN_EXPORT intptr_t GetSystemRecordingAvgBytesPerSec(void *client)
{
	if (client == nullptr)
		return 0;
	auto sareClient = S2IMPL(client);
	return sareClient->pWaveFormatEx->nAvgBytesPerSec;
}

FFI_PLUGIN_EXPORT intptr_t GetMicRecordingFormatTag(void *client)
{
	if (client == nullptr)
		return 0;
	auto sareClient = S2IMPL(client);
	return sareClient->MicWaveFormat.wFormatTag;
}

FFI_PLUGIN_EXPORT intptr_t GetMicRecordingChannels(void *client)
{
	if (client == nullptr)
		return 0;
	auto sareClient = S2IMPL(client);
	return sareClient->MicWaveFormat.nChannels;
}

FFI_PLUGIN_EXPORT intptr_t GetMicRecordingSamplesPerSec(void *client)
{
	if (client == nullptr)
		return 0;
	auto sareClient = S2IMPL(client);
	return sareClient->MicWaveFormat.nSamplesPerSec;
}

FFI_PLUGIN_EXPORT intptr_t GetMicRecordingBitsPerSample(void *client)
{
	if (client == nullptr)
		return 0;
	auto sareClient = S2IMPL(client);
	return sareClient->MicWaveFormat.wBitsPerSample;
}

FFI_PLUGIN_EXPORT intptr_t GetMicRecordingBlockAlign(void *client)
{
	if (client == nullptr)
		return 0;
	auto sareClient = S2IMPL(client);
	return sareClient->MicWaveFormat.nBlockAlign;
}

FFI_PLUGIN_EXPORT intptr_t GetMicRecordingAvgBytesPerSec(void *client)
{
	if (client == nullptr)
		return 0;
	auto sareClient = S2IMPL(client);
	return sareClient->MicWaveFormat.nAvgBytesPerSec;
}




void MuteMicrophone(bool mute) {
    HRESULT hr;
    IMMDeviceEnumerator* pEnumerator = NULL;
    IMMDevice* pDevice = NULL;
    IAudioEndpointVolume* pEndpointVolume = NULL;


    // Get the default capture endpoint
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pEnumerator));
    if (FAILED(hr)) {
        CoUninitialize();
        printf("Failed to create MMDeviceEnumerator!\n");
        return;
    }

    hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eCommunications, &pDevice);
    if (FAILED(hr)) {
        pEnumerator->Release();
        CoUninitialize();
        printf("Failed to get default audio endpoint!\n");
        return;
    }

    // Get the audio endpoint volume interface
    hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (void**)&pEndpointVolume);
    pDevice->Release();
    pEnumerator->Release();
    if (FAILED(hr)) {
        CoUninitialize();
        printf("Failed to get endpoint volume interface!\n");
        return;
    }

    // Set the microphone mute state
    hr = pEndpointVolume->SetMute(mute, NULL);
    pEndpointVolume->Release();
}





FFI_PLUGIN_EXPORT intptr_t IncludeMicRecording(void *client, intptr_t mute)
{
	if (client == nullptr)
		return 0;
	auto sareClient = S2IMPL(client);
	sareClient->muteMic = mute == 1;
	MuteMicrophone(mute==1);
	return 1;
}

FFI_PLUGIN_EXPORT intptr_t IsMicIncluded(void *client)
{
	if (client == nullptr)
		return 0;
	auto sareClient = S2IMPL(client);
	return sareClient->muteMic;
}

