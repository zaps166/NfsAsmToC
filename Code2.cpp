extern "C" void nfs2seEntrypoint()
{
	Game game, audio;
	main_game_thread = &game;
	audio_game_thread = &audio;
	game._start();
}

/* External functions */

#undef sub_41B250
extern "C" void sub_41B250(Game &game, int32_t arg1, int32_t arg2)
{
	game.eax = arg1;
	game.edx = arg2;
	game._sub_41B250();
}

extern "C" void wrap_regparm2(Game &game, void (*func)(Game &), int32_t arg0, int32_t arg1)
{
	game.eax = arg0;
	game.edx = arg1;
	func(game);
}

extern "C" int32_t wrap_stdcall2_ret(Game &game, void (*func)(Game &), int32_t arg0, int32_t arg1)
{
	game.push32(arg1);
	game.push32(arg0);
	game.esp -= 4;
	func(game);
	game.esp += 4;
	return game.eax;
}
extern "C" void wrap_stdcall4(Game &game, void (*func)(Game &), int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3)
{
	game.push32(arg3);
	game.push32(arg2);
	game.push32(arg1);
	game.push32(arg0);
	game.esp -= 4;
	func(game);
	game.esp += 4;
}

/* DInput */

#ifndef TESTS
#define WrapFunction1Arg(func_name) \
	extern "C" int32_t func_name(int32_t arg0); \
	extern "C" void func_name##_wrap(Game &game) \
	{ \
		game.eax = func_name(*(int32_t *)(game.esp + 4)); \
		game.esp += 4; \
	}
#define WrapFunction2Arg(func_name) \
	extern "C" int32_t func_name(int32_t arg0, int32_t arg1); \
	extern "C" void func_name##_wrap(Game &game) \
	{ \
		game.eax = func_name(*(int32_t *)(game.esp + 4), *(int32_t *)(game.esp + 8)); \
		game.esp += 8; \
	}
#define WrapFunction3Arg(func_name) \
	extern "C" int32_t func_name(int32_t arg0, int32_t arg1, int32_t arg2); \
	extern "C" void func_name##_wrap(Game &game) \
	{ \
		game.eax = func_name(*(int32_t *)(game.esp + 4), *(int32_t *)(game.esp + 8), *(int32_t *)(game.esp + 12)); \
		game.esp += 12; \
	}
#define WrapFunction4Arg(func_name) \
	extern "C" int32_t func_name(int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3); \
	extern "C" void func_name##_wrap(Game &game) \
	{ \
		game.eax = func_name(*(int32_t *)(game.esp + 4), *(int32_t *)(game.esp + 8), *(int32_t *)(game.esp + 12), *(int32_t *)(game.esp + 16)); \
		game.esp += 16; \
	}
#define WrapFunction5Arg(func_name) \
	extern "C" int32_t func_name(int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4); \
	extern "C" void func_name##_wrap(Game &game) \
	{ \
		game.eax = func_name(*(int32_t *)(game.esp + 4), *(int32_t *)(game.esp + 8), *(int32_t *)(game.esp + 12), *(int32_t *)(game.esp + 16), *(int32_t *)(game.esp + 20)); \
		game.esp += 20; \
	}

WrapFunction5Arg(EnumDevices)
WrapFunction4Arg(CreateDevice)
WrapFunction1Arg(Release)

WrapFunction3Arg(QueryInterface)

WrapFunction2Arg(GetCapabilities)
WrapFunction3Arg(SetProperty)
WrapFunction1Arg(Acquire)
WrapFunction1Arg(Unacquire)
WrapFunction3Arg(GetDeviceState)
WrapFunction5Arg(GetDeviceData)
WrapFunction2Arg(SetDataFormat)
WrapFunction2Arg(SetEventNotification)
WrapFunction3Arg(SetCooperativeLevel)
WrapFunction4Arg(GetObjectInfo)
WrapFunction5Arg(CreateEffect)
WrapFunction2Arg(SendForceFeedbackCommand)
WrapFunction1Arg(Poll)

WrapFunction3Arg(SetParameters)
WrapFunction3Arg(Start)
WrapFunction1Arg(Stop)
WrapFunction1Arg(Download)
WrapFunction1Arg(Unload)
#endif

/* External variables (as pointers) */

#undef dword_4DDA70
#undef dword_5637CC
#undef dword_5637D8
#undef dword_4DB1B0
#undef dword_5637A0
void **dword_4DDA70 = (void **)&_data.dword_4DDA70, **dword_5637CC = (void **)&_bss.dword_5637CC, **dword_5637D8 = (void **)&_bss.dword_5637D8;
void *dword_4DB1B0 = (void *)&_data.dword_4DB1B0, *dword_5637A0 = (void *)&_bss.dword_5637A0;

#undef mousePositionX
#undef mousePositionY
void *mousePositionX = (void *)&_bss.mousePositionX;
void *mousePositionY = (void *)&_bss.mousePositionY;

#undef binaryGameVersion
void *binaryGameVersion = (void *)&_data.binaryGameVersion;
