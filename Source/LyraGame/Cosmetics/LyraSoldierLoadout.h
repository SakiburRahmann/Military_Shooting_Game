// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LyraSoldierLoadout.generated.h"

// Persistent soldier appearance: part variants + uniform color.
// Saved per player in ULyraSettingsLocal, replicated via ALyraPlayerState.
USTRUCT(BlueprintType)
struct FLyraSoldierLoadout
{
	GENERATED_BODY()

	// A02 Head variant 0-3
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config)
	int32 HeadVariant = 0;

	// A02 Chest variant 1-3
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config)
	int32 ChestVariant = 1;

	// A02 Hands variant 1-3
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config)
	int32 HandVariant = 1;

	// A02 Legs variant 1-3
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config)
	int32 LegVariant = 1;

	// A02 Feet variant (only 1 exists)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config)
	int32 FootVariant = 1;

	// A02 Helmet 0=none, 1-3 variants
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config)
	int32 HelmetVariant = 1;

	// Index into GetSoldierUniformColors()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config)
	int32 UniformColorIndex = 0;

	static FLyraSoldierLoadout MakeDefault()
	{
		FLyraSoldierLoadout Out;
		Out.HeadVariant = 0;
		Out.ChestVariant = 1;
		Out.HandVariant = 1;
		Out.LegVariant = 1;
		Out.FootVariant = 1;
		Out.HelmetVariant = 1;
		Out.UniformColorIndex = 0;
		return Out;
	}

	static TArray<FLinearColor> GetSoldierUniformColors()
	{
		return {
			FLinearColor(0.32f, 0.30f, 0.20f), // Olive drab (default)
			FLinearColor(0.55f, 0.48f, 0.34f), // Desert tan
			FLinearColor(0.08f, 0.08f, 0.09f), // Black ops
			FLinearColor(0.35f, 0.36f, 0.38f), // Urban grey
			FLinearColor(0.16f, 0.20f, 0.26f), // Midnight navy
			FLinearColor(0.45f, 0.42f, 0.36f), // Ranger khaki
		};
	}

	static FLinearColor ResolveUniformColor(int32 Index)
	{
		const TArray<FLinearColor> Colors = GetSoldierUniformColors();
		if (Colors.IsValidIndex(Index))
		{
			return Colors[Index];
		}
		return Colors[0];
	}
};
