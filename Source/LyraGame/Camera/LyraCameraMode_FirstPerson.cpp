// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraCameraMode_FirstPerson.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraCameraMode_FirstPerson)

ULyraCameraMode_FirstPerson::ULyraCameraMode_FirstPerson()
{
	FieldOfView = 90.0f;
	BlendTime = 0.1f;
}

void ULyraCameraMode_FirstPerson::UpdateView(float DeltaTime)
{
	FVector PivotLocation = FVector::ZeroVector;
	bool bFoundSocket = false;

	const AActor* TargetActor = GetTargetActor();
	if (const ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
	{
		if (const USkeletalMeshComponent* Mesh = TargetCharacter->GetMesh())
		{
			if (Mesh->DoesSocketExist(TEXT("head")))
			{
				PivotLocation = Mesh->GetSocketLocation(TEXT("head"));
				bFoundSocket = true;
			}
		}
	}

	if (!bFoundSocket)
	{
		PivotLocation = GetPivotLocation();
	}

	FRotator PivotRotation = GetPivotRotation();
	PivotRotation.Pitch = FMath::ClampAngle(PivotRotation.Pitch, ViewPitchMin, ViewPitchMax);

	// Precise biological eye alignment relative to the animated head socket:
	// The eyes are located approximately 12 cm forward and 5 cm upward relative to the head joint center.
	// Offsetting along the looking direction ensures we stay in front of the head mesh and face plane.
	FVector ForwardOffset = PivotRotation.Vector() * 12.0f;
	FVector UpwardOffset = FVector::UpVector * 5.0f;

	View.Location = PivotLocation + ForwardOffset + UpwardOffset;
	View.Rotation = PivotRotation;
	View.ControlRotation = View.Rotation;
	View.FieldOfView = FieldOfView;
}

void ULyraCameraMode_FirstPerson::OnActivation()
{
	Super::OnActivation();
}

void ULyraCameraMode_FirstPerson::OnDeactivation()
{
	Super::OnDeactivation();
}
