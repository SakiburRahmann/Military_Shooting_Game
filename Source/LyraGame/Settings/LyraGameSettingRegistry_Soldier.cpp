// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameSettingCollection.h"
#include "GameSettingValueDiscreteDynamic.h"
#include "DataSource/GameSettingDataSourceDynamic.h"
#include "EditCondition/WhenCondition.h"
#include "LyraGameSettingRegistry.h"
#include "LyraSettingsLocal.h"
#include "Player/LyraLocalPlayer.h"
#include "Cosmetics/LyraSoldierLoadout.h"

#define LOCTEXT_NAMESPACE "Lyra"

namespace LyraSoldierSettings
{
	UGameSettingValueDiscreteDynamic* MakeVariantSetting(
		const TCHAR* DevName, const FText& DisplayName, const FText& Description,
		const TSharedRef<FGameSettingDataSourceDynamic>& Getter,
		const TSharedRef<FGameSettingDataSourceDynamic>& Setter,
		const TArray<TPair<FString, FText>>& Options, const FString& DefaultValue)
	{
		UGameSettingValueDiscreteDynamic* Setting = NewObject<UGameSettingValueDiscreteDynamic>();
		Setting->SetDevName(DevName);
		Setting->SetDisplayName(DisplayName);
		Setting->SetDescriptionRichText(Description);
		Setting->SetDynamicGetter(Getter);
		Setting->SetDynamicSetter(Setter);
		Setting->SetDefaultValueFromString(DefaultValue);
		for (const TPair<FString, FText>& Option : Options)
		{
			Setting->AddDynamicOption(Option.Key, Option.Value);
		}
		return Setting;
	}
}

UGameSettingCollection* ULyraGameSettingRegistry::InitializeSoldierSettings(ULyraLocalPlayer* InLocalPlayer)
{
	UGameSettingCollection* Screen = NewObject<UGameSettingCollection>();
	Screen->SetDevName(TEXT("SoldierCollection"));
	Screen->SetDisplayName(LOCTEXT("SoldierCollection_Name", "Soldier"));
	Screen->Initialize(InLocalPlayer);

	// Soldier customization is a pre-match activity: hide it while possessing
	// a pawn (in-match pause menu), show it in the frontend.
	const TSharedRef<FWhenCondition> WhenInFrontend = MakeShared<FWhenCondition>(
		[](const ULocalPlayer* InLocalPlayer, FGameSettingEditableState& InOutEditState)
		{
			if (const APlayerController* PC = InLocalPlayer->PlayerController)
			{
				if (PC->GetPawn() != nullptr)
				{
					InOutEditState.Kill(TEXT("Soldier customization is only available in the main menu"));
				}
			}
		});

	UGameSettingCollection* Appearance = NewObject<UGameSettingCollection>();
	Appearance->SetDevName(TEXT("SoldierAppearance"));
	Appearance->SetDisplayName(LOCTEXT("SoldierAppearance_Name", "Appearance"));
	Screen->AddSetting(Appearance);

	auto AddSoldierSetting = [&](UGameSetting* Setting)
	{
		Setting->AddEditCondition(WhenInFrontend);
		Appearance->AddSetting(Setting);
	};

	//----------------------------------------------------------------------------------
	{
		AddSoldierSetting(LyraSoldierSettings::MakeVariantSetting(
			TEXT("SoldierHead"), LOCTEXT("SoldierHead_Name", "Head"),
			LOCTEXT("SoldierHead_Description", "Head variant for your soldier."),
			GET_LOCAL_SETTINGS_FUNCTION_PATH(GetSoldierHead),
			GET_LOCAL_SETTINGS_FUNCTION_PATH(SetSoldierHead),
			{ {TEXT("0"), LOCTEXT("SoldierHead0", "Head 1")}, {TEXT("1"), LOCTEXT("SoldierHead1", "Head 2")},
			  {TEXT("2"), LOCTEXT("SoldierHead2", "Head 3")}, {TEXT("3"), LOCTEXT("SoldierHead3", "Head 4")} },
			TEXT("0")));
	}
	//----------------------------------------------------------------------------------
	{
		AddSoldierSetting(LyraSoldierSettings::MakeVariantSetting(
			TEXT("SoldierChest"), LOCTEXT("SoldierChest_Name", "Vest"),
			LOCTEXT("SoldierChest_Description", "Tactical vest variant for your soldier."),
			GET_LOCAL_SETTINGS_FUNCTION_PATH(GetSoldierChest),
			GET_LOCAL_SETTINGS_FUNCTION_PATH(SetSoldierChest),
			{ {TEXT("1"), LOCTEXT("SoldierChest1", "Vest 1")}, {TEXT("2"), LOCTEXT("SoldierChest2", "Vest 2")},
			  {TEXT("3"), LOCTEXT("SoldierChest3", "Vest 3")} },
			TEXT("1")));
	}
	//----------------------------------------------------------------------------------
	{
		AddSoldierSetting(LyraSoldierSettings::MakeVariantSetting(
			TEXT("SoldierHands"), LOCTEXT("SoldierHands_Name", "Gloves"),
			LOCTEXT("SoldierHands_Description", "Glove variant for your soldier."),
			GET_LOCAL_SETTINGS_FUNCTION_PATH(GetSoldierHands),
			GET_LOCAL_SETTINGS_FUNCTION_PATH(SetSoldierHands),
			{ {TEXT("1"), LOCTEXT("SoldierHands1", "Gloves 1")}, {TEXT("2"), LOCTEXT("SoldierHands2", "Gloves 2")},
			  {TEXT("3"), LOCTEXT("SoldierHands3", "Gloves 3")} },
			TEXT("1")));
	}
	//----------------------------------------------------------------------------------
	{
		AddSoldierSetting(LyraSoldierSettings::MakeVariantSetting(
			TEXT("SoldierLegs"), LOCTEXT("SoldierLegs_Name", "Pants"),
			LOCTEXT("SoldierLegs_Description", "Pants variant for your soldier."),
			GET_LOCAL_SETTINGS_FUNCTION_PATH(GetSoldierLegs),
			GET_LOCAL_SETTINGS_FUNCTION_PATH(SetSoldierLegs),
			{ {TEXT("1"), LOCTEXT("SoldierLegs1", "Pants 1")}, {TEXT("2"), LOCTEXT("SoldierLegs2", "Pants 2")},
			  {TEXT("3"), LOCTEXT("SoldierLegs3", "Pants 3")} },
			TEXT("1")));
	}
	//----------------------------------------------------------------------------------
	{
		AddSoldierSetting(LyraSoldierSettings::MakeVariantSetting(
			TEXT("SoldierHelmet"), LOCTEXT("SoldierHelmet_Name", "Headgear"),
			LOCTEXT("SoldierHelmet_Description", "Helmet for your soldier, or none."),
			GET_LOCAL_SETTINGS_FUNCTION_PATH(GetSoldierHelmet),
			GET_LOCAL_SETTINGS_FUNCTION_PATH(SetSoldierHelmet),
			{ {TEXT("0"), LOCTEXT("SoldierHelmet0", "None")}, {TEXT("1"), LOCTEXT("SoldierHelmet1", "Helmet 1")},
			  {TEXT("2"), LOCTEXT("SoldierHelmet2", "Helmet 2")}, {TEXT("3"), LOCTEXT("SoldierHelmet3", "Helmet 3")} },
			TEXT("1")));
	}
	return Screen;
}

#undef LOCTEXT_NAMESPACE
