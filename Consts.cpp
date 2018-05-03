#include "Consts.hpp"

const QMap<QString, ExternData> externs = {
	{"SDL_NumJoysticks_wrap", {}},
	{"SDL_GetTicks_wrap", {}},
	{"SDL_Delay_wrap", {1, ExternData::CDECL}},

	{"iSNDdllversion_", {}},
	{"iSNDdirectsetfunctions_wrap", {5, ExternData::STDCALL}},
	{"iSNDdirectcaps_", {1, ExternData::WATCOM_FASTCALL}},
	{"iSNDdirectstart_", {2, ExternData::WATCOM_FASTCALL}},
	{"iSNDdirectserve_", {1, ExternData::CDECL}}, //modified (original: 0 args)
	{"iSNDdirectstop_", {}},

	{"grAlphaBlendFunction", {4}},
	{"grAlphaCombine", {5}},
	{"grAlphaTestFunction", {1}},
	{"grAlphaTestReferenceValue", {1}},
	{"grBufferClear", {3}},
	{"grBufferSwap", {1}},
	{"grChromakeyMode", {1}},
	{"grChromakeyValue", {1}},
	{"grClipWindow", {4}},
	{"grColorCombine", {5}},
	{"grCullMode", {1}},
	{"grDepthBiasLevel", {1}},
	{"grDepthBufferFunction", {1}},
	{"grDepthBufferMode", {1}},
	{"grDepthMask", {1}},
	{"grDitherMode", {1}},
	{"grDrawLine", {2}},
	{"grDrawTriangle", {3}},
	{"grFogColorValue", {1}},
	{"grFogMode", {1}},
	{"grFogTable", {1}},
	{"grGammaCorrectionValue", {1, ExternData::STDCALL, {0}}},
	{"grGlideInit", {}},
	{"grGlideShutdown", {}},
	{"grLfbLock", {6}},
	{"grLfbUnlock", {2}},
	{"grRenderBuffer", {1}},
	{"grSstIdle", {}},
	{"grSstIsBusy", {}},
	{"grSstQueryHardware", {1}},
	{"grSstSelect", {1}},
	{"grSstStatus", {}},
	{"grSstWinClose", {}},
	{"grSstWinOpen", {7}},
	{"grTexCalcMemRequired", {4}},
	{"grTexClampMode", {3}},
	{"grTexCombine", {7}},
	{"grTexCombineFunction", {2}},
	{"grTexDownloadMipMap", {4}},
	{"grTexDownloadTable", {3}},
	{"grTexFilterMode", {3}},
	{"grTexMaxAddress", {1}},
	{"grTexMinAddress", {1}},
	{"grTexMipMapMode", {3}},
	{"grTexSource", {4}},
	{"guFogGenerateExp", {2, ExternData::STDCALL, {1}}},

	{"WrapperAtExit", {1, ExternData::STDCALL}},
	{"WrapperCreateWindow", {1, ExternData::STDCALL}},
	{"fetchTrackRecords", {3, ExternData::WATCOM_FASTCALL}}, //modified (original: 2 args)
	{"WrapperInit", {}},
	{"startTimer", {}},
	{"stopTimer", {}},

	{"vsprintf_wrap", {3, ExternData::CDECL}},
	{"fscanf_wrap", {2, ExternData::CDECL_VARARG}},
	{"fclose_wrap", {1, ExternData::CDECL}},
	{"calloc_wrap", {2, ExternData::CDECL}},
	{"malloc_wrap", {1, ExternData::CDECL}},
	{"fopen_wrap", {2, ExternData::WATCOM_FASTCALL}},
	{"time_wrap", {1, ExternData::CDECL}},
	{"free_wrap", {1, ExternData::CDECL}},

	{"DefWindowProcA_wrap", {4}},
	{"DestroyWindow_wrap", {1}},
	{"DispatchMessageA_wrap", {2}}, //modified (original: 1 arg)
	{"GetKeyboardType_wrap", {1}},
	{"GetMessageA_wrap", {4}},
	{"MessageBoxA_wrap", {4}},
	{"PostMessageA_wrap", {4}},
	{"SetForegroundWindow_wrap", {1}},
	{"ShowCursor_wrap", {1}},
	{"SystemParametersInfoA_wrap", {4}},

	{"CloseHandle_wrap", {1}},
	{"CreateEventA_wrap", {4}},
	{"CreateFileA_wrap", {7}},
	{"CreateFileMappingA_wrap", {6}},
	{"CreateThread_wrap", {6}},
	{"DeleteCriticalSection_wrap", {1}},
	{"DeleteFileA_wrap", {1}},
	{"DuplicateHandle_wrap", {7}},
	{"EnterCriticalSection_wrap", {1}},
	{"ExitProcess_wrap", {1}},
	{"FindClose_wrap", {1}},
	{"FindFirstFileA_wrap", {2}},
	{"FindNextFileA_wrap", {2}},
	{"FlushFileBuffers_wrap", {1}},
	{"GetCommState_wrap", {2}},
	{"GetCurrentDirectoryA_wrap", {2}},
	{"GetCurrentProcess_wrap", {}},
	{"GetCurrentThreadId_wrap", {}},
	{"GetCurrentThread_wrap", {}},
	{"GetFileAttributesA_wrap", {1}},
	{"GetFileSize_wrap", {2}},
	{"GetLastError_wrap", {}},
	{"GetModuleHandleA_wrap", {1}},
	{"GetOverlappedResult_wrap", {4}},
	{"GetSystemInfo_wrap", {1}},
	{"GlobalMemoryStatus_wrap", {1}},
	{"InitializeCriticalSection_wrap", {1}},
	{"LeaveCriticalSection_wrap", {1}},
	{"MapViewOfFile_wrap", {5}},
	{"PurgeComm_wrap", {2}},
	{"ReadFile_wrap", {5}},
	{"ResumeThread_wrap", {1}},
	{"SetCommState_wrap", {2}},
	{"SetCommTimeouts_wrap", {2}},
	{"SetCurrentDirectoryA_wrap", {1}},
	{"SetEndOfFile_wrap", {1}},
	{"SetEvent_wrap", {1}},
	{"SetFileAttributesA_wrap", {2}},
	{"SetFilePointer_wrap", {4}},
	{"SetThreadPriority_wrap", {2}},
	{"TerminateThread_wrap", {2}},
	{"UnmapViewOfFile_wrap", {1}},
	{"WaitForMultipleObjects_wrap", {4}},
	{"WriteFile_wrap", {5}},

	{"timeBeginPeriod_wrap", {1}},
	{"timeEndPeriod_wrap", {1}},
	{"timeGetDevCaps_wrap", {2}},

	{"DirectInputCreateA_wrap", {5}}, //modified (original: 4 args)

	{"inet_addr_wrap", {1}},
	{"listen_wrap", {2}},
	{"inet_ntoa_wrap", {1}},
	{"gethostbyname_wrap", {1}},
	{"gethostname_wrap", {2}},
	{"connect_wrap", {3}},
	{"accept_wrap", {3}},
	{"WSAFDIsSet_wrap", {2}},
	{"select_wrap", {5}},
	{"send_wrap", {4}},
	{"recv_wrap", {4}},
	{"sendto_wrap", {6}},
	{"getsockname_wrap", {3}},
	{"bind_wrap", {3}},
	{"htons_wrap", {1}},
	{"ioctlsocket_wrap", {3}},
	{"setsockopt_wrap", {5}},
	{"WSAGetLastError_wrap", {}},
	{"closesocket_wrap", {1}},
	{"socket_wrap", {3}},
	{"WSACleanup_wrap", {}},
	{"WSAStartup_wrap", {2}},
	{"recvfrom_wrap", {6}}
};