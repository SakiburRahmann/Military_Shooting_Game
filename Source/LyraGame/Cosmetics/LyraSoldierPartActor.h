// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystem/LyraTaggedActor.h"
#include "LyraSoldierPartActor.generated.h"

class UAnimInstance;
class USkeletalMesh;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class ESoldierPartSlot : uint8
{
	None,
	Head,
	Chest,
	Hands,
	Legs,
	Feet,
	Helmet
};

// One swappable soldier body part (head, chest, hands, ...).
// Spawned through the Lyra character-parts system; the CopyPose anim makes it
// follow the pawn's base mesh, so no per-part animation work is needed.
UCLASS(Blueprintable)
class ALyraSoldierPartActor : public ALyraTaggedActor
{
	GENERATED_BODY()

public:

	ALyraSoldierPartActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soldier")
	TObjectPtr<USkeletalMeshComponent> PartMesh;

	// Skeletal mesh rendered by this part (set on Blueprint children).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Soldier")
	TObjectPtr<USkeletalMesh> PartMeshAsset;

	// Copy-pose anim so the part follows the body (defaults to ABP_Mannequin_CopyPose).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Soldier")
	TSubclassOf<UAnimInstance> PartAnimClass;

	// Which body slot this part fills (used for loadouts + first-person head hiding).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Soldier")
	ESoldierPartSlot PartSlot = ESoldierPartSlot::None;

protected:

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void ApplyPartMesh();
};
