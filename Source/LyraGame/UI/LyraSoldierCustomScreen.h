// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UI/LyraActivatableWidget.h"
#include "LyraSoldierCustomScreen.generated.h"

class UButton;
class UHorizontalBox;
class UTextBlock;
class UVerticalBox;

// Main-menu "Customize Character" screen. Builds its rows in code so no
// widget-blueprint authoring is required. Writes straight to the local
// soldier loadout (saved to disk) for the next spawned soldier.
UCLASS(Blueprintable)
class ULyraSoldierCustomScreen : public ULyraActivatableWidget
{
	GENERATED_BODY()

public:

	ULyraSoldierCustomScreen(const FObjectInitializer& ObjectInitializer);

protected:

	virtual void NativeOnInitialized() override;

private:

	UFUNCTION()
	void HandleBackClicked();

	// Opens the customization screen on the menu layer (for main-menu buttons).
	UFUNCTION(BlueprintCallable, Category = "Soldier", meta = (WorldContext = "WorldContextObject"))
	static void OpenSoldierCustomization(UObject* WorldContextObject);

	UFUNCTION()
	void HandleSlotPrev0();
	UFUNCTION()
	void HandleSlotNext0();
	UFUNCTION()
	void HandleSlotPrev1();
	UFUNCTION()
	void HandleSlotNext1();
	UFUNCTION()
	void HandleSlotPrev2();
	UFUNCTION()
	void HandleSlotNext2();
	UFUNCTION()
	void HandleSlotPrev3();
	UFUNCTION()
	void HandleSlotNext3();
	UFUNCTION()
	void HandleSlotPrev4();
	UFUNCTION()
	void HandleSlotNext4();

	void CycleSlot(int32 RowIndex, int32 Direction);
	void RefreshRowTexts();
	int32 GetRowValue(int32 RowIndex) const;
	void SetRowValue(int32 RowIndex, int32 Value);
	FText GetRowOptionLabel(int32 RowIndex, int32 Value) const;

	UPROPERTY()
	TObjectPtr<UVerticalBox> RootBox;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> RowValueTexts;
};
