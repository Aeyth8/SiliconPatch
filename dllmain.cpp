#include "pch.h"
#include "Aeyth8/Global.hpp"
#include "Aeyth8/Tools/Pointers.h"
#include "Aeyth8/Hooks/Hooks.hpp"
#include "Aeyth8/Tools/BytePatcher.h"

#include "Dumper-7/SDK/EngineSettings_classes.hpp"
#include "Dumper-7/SDK/BP_CharacterBase_classes.hpp"
#include "Dumper-7/SDK/UMG_classes.hpp"
#include "Dumper-7/SDK/VerifyGame_classes.hpp"

using namespace A8CL; using namespace Global; using namespace Pointers;

A8CL::OFFSET ProcessEvent("UObject::ProcessEvent", 0x819B70);
A8CL::OFFSET Invoke("UFunction::Invoke", 0x702780);
A8CL::OFFSET Entry("WinMainCRTStartup", 0x1F5DA38);

SDK::UGameMapsSettings* MapSettings{nullptr};

static SDK::UEngine* const& GEngine(const bool bLog = false)
{
	SDK::UEngine*& Engine = *reinterpret_cast<SDK::UEngine**>(PB(0x31F7858));
	if (bLog && IsNull(Engine))
	{
		LogA("Logic", "GEngine is a null pointer!");
	}
	return Engine;
}

static void PreInit()
{
	GBA = (uintptr_t)GetModuleHandleA("SiliconRising-Win64-Shipping.exe");
	//LogWin();
	LogA("Initialized", "The [GBA] Global Base Address is " + HexToString(GBA));

	BytePatcher::ReplaceBytes(PB(0x531450), {0xB0, 0x01, 0xC3, 0x90}); // AViveportSDK::GetIsDRMCheckOK (no clue if this does anything)
	BytePatcher::ReplaceBytes(PB(0x124EECB), {0x41, 0xC6, 0x01, 0x01, 0x90}); // Forces bMoviesAreSkippable to be true, still plays the sequence.
	BytePatcher::ReplaceBytes(PB(0x539B50), {0xC3, 0x90}); // AMercenary2GameMode::VerifyUserInfo
}

/*
AES - 97 56 07 D8 A8 67 3D 52 D9 B3 24 E9 5F 56 40 54 E7 D7 C4 F1 A9 C1 CA 96 7C CB 10 AB 5D 6C 55 10
AES IV - DC F1 58 18 CA 11 B1 30 2D 1F F8 C4 FB 05 A8 D2
*/

// 0x31F7858 GEngine
static void Init()
{
	while (GEngine() == nullptr) Sleep(150);

	if (!IsNull(MapSettings = SDK::UGameMapsSettings::GetDefaultObj()))
	{
		MapSettings->GameDefaultMap.AssetPathName = FString2FName(L"/Game/Maps/MainMenuLevel_Online.MainMenuLevel_Online");
	}

	while (UWorld(false) == nullptr) Sleep(1000);
	
	if (!bConstructedUConsole) bConstructedUConsole = ConstructUConsole();

	while (1)
	{
		Sleep(1750);

		SDK::APlayerController* PC = Player();
		if (PC && PC->Character && PC->Character->IsA(SDK::ABP_CharacterBase_C::StaticClass()))
		{
			SDK::ABP_CharacterBase_C* Character = static_cast<SDK::ABP_CharacterBase_C*>(PC->Character);

			if (Character->VerifyGame)
			{
				SDK::UUserWidget* ErrorScreen = Character->VerifyGame->GetUserWidgetObject();
				if (ErrorScreen)
				{
					ErrorScreen->SetVisibility(SDK::ESlateVisibility::Hidden);
				}
			}
		}
	}
}

int __stdcall DllMain(HMODULE hModule, DWORD ulReasonForCall, LPVOID lpReserved)
{
	DisableThreadLibraryCalls(hModule);

	if (ulReasonForCall == DLL_PROCESS_ATTACH)
	{
		InitLog();
		PreInit();

		if (Proxy::Attach(hModule))
			ConstructThread(Init);
	}

	return 1;
}