// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/LyraSoldierCustomScreen.h"

#include "Components/Button.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Cosmetics/LyraSoldierLoadout.h"
#include "Player/LyraLocalPlayer.h"
#include "Settings/LyraSettingsLocal.h"
#include "CommonUIExtensions.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraSoldierCustomScreen)

void ULyraSoldierCustomScreen::OpenSoldierCustomization(UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
	if (!LP)
	{
		return;
	}
	const FGameplayTag MenuLayer = FGameplayTag::RequestGameplayTag(TEXT("UI.Layer.Menu"));
	UCommonUIExtensions::PushContentToLayer_ForPlayer(LP, MenuLayer, ULyraSoldierCustomScreen::StaticClass());
}

namespace LyraSoldierScreen
{
	const int32 RowMin[5] = { 0, 1, 1, 1, 0 };
	const int32 RowMax[5] = { 3, 3, 3, 3, 3 };

	const TCHAR* RowLabel(int32 Row)
	{
		switch (Row)
		{
		case 0: return TEXT("Head");
		case 1: return TEXT("Vest");
		case 2: return TEXT("Gloves");
		case 3: return TEXT("Pants");
		case 4: return TEXT("Headgear");
		default: return TEXT("");
		}
	}
}

ULyraSoldierCustomScreen::ULyraSoldierCustomScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ULyraSoldierCustomScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RootBox = NewObject<UVerticalBox>(this, TEXT("RootBox"));
	if (UWidget* RootWidget = GetRootWidget())
	{
		// Replace content when hosted inside a designer shell; otherwise take over.
		if (UPanelWidget* RootPanel = Cast<UPanelWidget>(RootWidget))
		{
			RootPanel->ClearChildren();
			RootPanel->AddChild(RootBox);
		}
	}
	else if (UWidgetTree* Tree = WidgetTree)
	{
		// Code-only widget with no designer root: install our box as the
		// root, otherwise everything built below stays orphaned and the
		// screen opens empty.
		Tree->RootWidget = RootBox;
	}

	auto AddText = [this](const FText& InText, float FontSize) -> UTextBlock*
	{
		UTextBlock* TB = NewObject<UTextBlock>(this);
		TB->SetText(InText);
		TB->SetFontSize(FontSize);
		return TB;
	};

	auto AddArrowButton = [this](const TCHAR* Label) -> UButton*
	{
		UButton* Btn = NewObject<UButton>(this);
		UTextBlock* TB = NewObject<UTextBlock>(this);
		TB->SetText(FText::FromString(Label));
		TB->SetFontSize(28);
		Btn->SetContent(TB);
		return Btn;
	};

	if (!RootBox)
	{
		return;
	}

	UTextBlock* Title = AddText(FText::FromString(TEXT("CUSTOMIZE CHARACTER")), 40);
	if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 20.f));
	}

	RowValueTexts.SetNumZeroed(5);
	TArray<TObjectPtr<UButton>> PrevButtons;
	TArray<TObjectPtr<UButton>> NextButtons;
	PrevButtons.SetNumZeroed(5);
	NextButtons.SetNumZeroed(5);

	for (int32 Row = 0; Row < 5; ++Row)
	{
		UHorizontalBox* HBox = NewObject<UHorizontalBox>(this);
		if (UVerticalBoxSlot* RowSlot = RootBox->AddChildToVerticalBox(HBox))
		{
			RowSlot->SetPadding(FMargin(0.f, 6.f));
		}

		UTextBlock* Label = AddText(FText::FromString(LyraSoldierScreen::RowLabel(Row)), 24);
		if (UHorizontalBoxSlot* LabelSlot = HBox->AddChildToHorizontalBox(Label))
		{
			LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		PrevButtons[Row] = AddArrowButton(TEXT("<"));
		HBox->AddChildToHorizontalBox(PrevButtons[Row]);

		UTextBlock* Value = AddText(FText::GetEmpty(), 24);
		RowValueTexts[Row] = Value;
		if (UHorizontalBoxSlot* ValueSlot = HBox->AddChildToHorizontalBox(Value))
		{
			ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		NextButtons[Row] = AddArrowButton(TEXT(">"));
		HBox->AddChildToHorizontalBox(NextButtons[Row]);
	}

	PrevButtons[0]->OnClicked.AddDynamic(this, &ULyraSoldierCustomScreen::HandleSlotPrev0);
	NextButtons[0]->OnClicked.AddDynamic(this, &ULyraSoldierCustomScreen::HandleSlotNext0);
	PrevButtons[1]->OnClicked.AddDynamic(this, &ULyraSoldierCustomScreen::HandleSlotPrev1);
	NextButtons[1]->OnClicked.AddDynamic(this, &ULyraSoldierCustomScreen::HandleSlotNext1);
	PrevButtons[2]->OnClicked.AddDynamic(this, &ULyraSoldierCustomScreen::HandleSlotPrev2);
	NextButtons[2]->OnClicked.AddDynamic(this, &ULyraSoldierCustomScreen::HandleSlotNext2);
	PrevButtons[3]->OnClicked.AddDynamic(this, &ULyraSoldierCustomScreen::HandleSlotPrev3);
	NextButtons[3]->OnClicked.AddDynamic(this, &ULyraSoldierCustomScreen::HandleSlotNext3);
	PrevButtons[4]->OnClicked.AddDynamic(this, &ULyraSoldierCustomScreen::HandleSlotPrev4);
	NextButtons[4]->OnClicked.AddDynamic(this, &ULyraSoldierCustomScreen::HandleSlotNext4);

	UButton* BackBtn = AddArrowButton(TEXT("BACK"));
	BackBtn->OnClicked.AddDynamic(this, &ULyraSoldierCustomScreen::HandleBackClicked);
	if (UVerticalBoxSlot* BackSlot = RootBox->AddChildToVerticalBox(BackBtn))
	{
		BackSlot->SetPadding(FMargin(0.f, 20.f, 0.f, 0.f));
	}

	RefreshRowTexts();
}

void ULyraSoldierCustomScreen::HandleBackClicked()
{
	DeactivateWidget();
}

#define SOLDIER_SLOT_HANDLERS(N) \
	void ULyraSoldierCustomScreen::HandleSlotPrev##N() { CycleSlot(N, -1); } \
	void ULyraSoldierCustomScreen::HandleSlotNext##N() { CycleSlot(N, +1); }

SOLDIER_SLOT_HANDLERS(0)
SOLDIER_SLOT_HANDLERS(1)
SOLDIER_SLOT_HANDLERS(2)
SOLDIER_SLOT_HANDLERS(3)
SOLDIER_SLOT_HANDLERS(4)

void ULyraSoldierCustomScreen::CycleSlot(int32 RowIndex, int32 Direction)
{
	const int32 Min = LyraSoldierScreen::RowMin[RowIndex];
	const int32 Max = LyraSoldierScreen::RowMax[RowIndex];
	int32 Value = GetRowValue(RowIndex) + Direction;
	if (Value < Min)
	{
		Value = Max;
	}
	if (Value > Max)
	{
		Value = Min;
	}
	SetRowValue(RowIndex, Value);
	RefreshRowTexts();
}

int32 ULyraSoldierCustomScreen::GetRowValue(int32 RowIndex) const
{
	FLyraSoldierLoadout Loadout = FLyraSoldierLoadout::MakeDefault();
	if (const ULyraLocalPlayer* LP = Cast<ULyraLocalPlayer>(GetOwningLocalPlayer()))
	{
		if (const ULyraSettingsLocal* Settings = LP->GetLocalSettings())
		{
			Loadout = Settings->GetSoldierLoadout();
		}
	}
	switch (RowIndex)
	{
	case 0: return Loadout.HeadVariant;
	case 1: return Loadout.ChestVariant;
	case 2: return Loadout.HandVariant;
	case 3: return Loadout.LegVariant;
	case 4: return Loadout.HelmetVariant;
	default: return 0;
	}
}

void ULyraSoldierCustomScreen::SetRowValue(int32 RowIndex, int32 Value)
{
	ULyraLocalPlayer* LP = Cast<ULyraLocalPlayer>(GetOwningLocalPlayer());
	ULyraSettingsLocal* Settings = LP ? LP->GetLocalSettings() : nullptr;
	if (!Settings)
	{
		return;
	}
	FLyraSoldierLoadout Loadout = Settings->GetSoldierLoadout();
	switch (RowIndex)
	{
	case 0: Loadout.HeadVariant = Value; break;
	case 1: Loadout.ChestVariant = Value; break;
	case 2: Loadout.HandVariant = Value; break;
	case 3: Loadout.LegVariant = Value; break;
	case 4: Loadout.HelmetVariant = Value; break;
	default: return;
	}
	Settings->SetSoldierLoadout(Loadout);
	Settings->SaveSettings();
}

FText ULyraSoldierCustomScreen::GetRowOptionLabel(int32 RowIndex, int32 Value) const
{
	switch (RowIndex)
	{
	case 0: return FText::Format(FText::FromString(TEXT("Head {0}")), Value + 1);
	case 1: return FText::Format(FText::FromString(TEXT("Vest {0}")), Value);
	case 2: return FText::Format(FText::FromString(TEXT("Gloves {0}")), Value);
	case 3: return FText::Format(FText::FromString(TEXT("Pants {0}")), Value);
	case 4: return Value <= 0 ? FText::FromString(TEXT("None")) : FText::Format(FText::FromString(TEXT("Helmet {0}")), Value);
	default: return FText::GetEmpty();
	}
}

void ULyraSoldierCustomScreen::RefreshRowTexts()
{
	for (int32 Row = 0; Row < RowValueTexts.Num(); ++Row)
	{
		if (RowValueTexts[Row])
		{
			RowValueTexts[Row]->SetText(GetRowOptionLabel(Row, GetRowValue(Row)));
		}
	}
}
