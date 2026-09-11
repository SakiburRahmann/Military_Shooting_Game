// Copyright Epic Games, Inc. All Rights Reserved.

#include "Cosmetics/LyraSoldierPartActor.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraSoldierPartActor)

ALyraSoldierPartActor::ALyraSoldierPartActor()
{
	PartMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PartMesh"));
	check(PartMesh);
	SetRootComponent(PartMesh);
	PartMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PartMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	// Parts get hidden in first person / ADS; keep their anims ticking while
	// hidden so run/reload/jump never stall on unhide.
	PartMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	static ConstructorHelpers::FClassFinder<UAnimInstance> CopyPoseFinder(
		TEXT("/Game/Characters/Heroes/Mannequin/Animations/ABP_Mannequin_CopyPose"));
	if (CopyPoseFinder.Succeeded())
	{
		PartAnimClass = CopyPoseFinder.Class;
	}
}

void ALyraSoldierPartActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyPartMesh();
}

void ALyraSoldierPartActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyPartMesh();
}

void ALyraSoldierPartActor::ApplyPartMesh()
{
	if (!PartMesh)
	{
		return;
	}
	if (PartMeshAsset && PartMesh->GetSkeletalMeshAsset() != PartMeshAsset)
	{
		PartMesh->SetSkeletalMesh(PartMeshAsset);
	}
	if (PartAnimClass && (!PartMesh->GetAnimInstance() || !PartMesh->GetAnimInstance()->GetClass()->IsChildOf(PartAnimClass)))
	{
		PartMesh->SetAnimInstanceClass(PartAnimClass);
	}
}
