// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraCameraMode_FirstPerson.h"
#include "Camera/LyraCameraMode.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraCameraMode_FirstPerson)

ULyraCameraMode_FirstPerson::ULyraCameraMode_FirstPerson()
{
	FieldOfView = 90.0f;
	BlendTime = 0.1f;
}

void ULyraCameraMode_FirstPerson::UpdateView(float DeltaTime)
{
	FVector PivotLocation = FVector::ZeroVector;
	bool bFoundHead = false;

	if (const ACharacter* TargetCharacter = Cast<ACharacter>(GetTargetActor()))
	{
		if (USkeletalMeshComponent* Mesh = TargetCharacter->GetMesh())
		{
			// GetSocketLocation also resolves bone names, so no head socket asset is required.
			// The bone tracks skeletal animation, so the camera rides run/sprint correctly.
			if (Mesh->GetBoneIndex(HeadBoneName) != INDEX_NONE)
			{
				PivotLocation = Mesh->GetSocketLocation(HeadBoneName);
				bFoundHead = true;
			}
		}
	}

	if (!bFoundHead)
	{
		PivotLocation = GetPivotLocation();
	}

	FRotator PivotRotation = GetPivotRotation();
	PivotRotation.Pitch = FMath::ClampAngle(PivotRotation.Pitch, ViewPitchMin, ViewPitchMax);

	View.Location = PivotLocation + (PivotRotation.Vector() * EyeForwardOffset) + (FVector::UpVector * EyeUpOffset);
	View.Rotation = PivotRotation;
	View.ControlRotation = View.Rotation;
	View.FieldOfView = FieldOfView;
}

ULyraCameraMode_FirstPersonADS::ULyraCameraMode_FirstPersonADS()
{
	FieldOfView = 55.0f;
	EyeForwardOffset = 20.0f;
	EyeUpOffset = 3.0f;
	BlendTime = 0.15f;
	BlendFunction = ELyraCameraModeBlendFunction::EaseInOut;
}
