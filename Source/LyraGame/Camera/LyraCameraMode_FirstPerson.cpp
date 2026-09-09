// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraCameraMode_FirstPerson.h"
#include "Camera/LyraCameraMode.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Character/LyraHealthComponent.h"
#include "LyraLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraCameraMode_FirstPerson)

ULyraCameraMode_FirstPerson::ULyraCameraMode_FirstPerson()
{
	FieldOfView = 90.0f;
	BlendTime = 0.1f;
}

// Shared handle so any camera mode can kill a scope widget left behind by a
// destroyed pawn (dying mid-ADS never runs ADS deactivation).
static TWeakObjectPtr<UUserWidget> GActiveScopeWidget;

void ULyraCameraMode_FirstPerson::OnActivation()
{
	// New life/mode: drop any scope overlay orphaned by death.
	if (UUserWidget* Stale = GActiveScopeWidget.Get())
	{
		Stale->SetVisibility(ESlateVisibility::Collapsed);
		Stale->RemoveFromParent();
		GActiveScopeWidget = nullptr;
	}
}

void ULyraCameraMode_FirstPerson::UpdateView(float DeltaTime)
{
	FVector PivotLocation = FVector::ZeroVector;
	bool bFoundHead = false;

	if (const ACharacter* TargetCharacter = Cast<ACharacter>(GetTargetActor()))
	{
		// Dead/dying: do NOT ride the head bone. It collapses to the floor with
		// the death anim and drags the camera under the ground (shake/fight).
		// Use the stable capsule pivot so death stays smooth.
		bool bDead = false;
		if (const ULyraHealthComponent* HealthComp = TargetCharacter->FindComponentByClass<ULyraHealthComponent>())
		{
			bDead = HealthComp->IsDeadOrDying();
		}

		if (!bDead)
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
	FieldOfView = 60.0f;
	EyeForwardOffset = 12.0f;
	EyeUpOffset = 5.0f;
	BlendTime = 0.15f;
	BlendFunction = ELyraCameraModeBlendFunction::EaseInOut;

	static ConstructorHelpers::FClassFinder<UUserWidget> CrosshairFinder(TEXT("/Game/UI/W_ADSCrosshair"));
	if (CrosshairFinder.Succeeded())
	{
		CrosshairWidgetClass = CrosshairFinder.Class;
	}
}

void ULyraCameraMode_FirstPersonADS::UpdateView(float DeltaTime)
{
	// Dead or dying: drop the scope and show the normal view so the death
	// animation plays instead of staying scoped.
	if (const ACharacter* TargetCharacter = Cast<ACharacter>(GetTargetActor()))
	{
		if (const ULyraHealthComponent* HealthComp = TargetCharacter->FindComponentByClass<ULyraHealthComponent>())
		{
			if (HealthComp->IsDeadOrDying())
			{
				SetADSCrosshairVisible(false);
				SetADSFirstPersonMeshesHidden(false);
				ULyraCameraMode_FirstPerson::UpdateView(DeltaTime);
				View.FieldOfView = 90.0f;
				return;
			}
		}
	}

	// Same head-anchored FP view as hip fire; the ADS crosshair overlay carries
	// the aim read. Nothing here touches the gun or the body.
	ULyraCameraMode_FirstPerson::UpdateView(DeltaTime);
	View.FieldOfView = FieldOfView;
}

void ULyraCameraMode_FirstPersonADS::OnActivation()
{
	SetADSCrosshairVisible(true);
	SetADSFirstPersonMeshesHidden(true);
}

void ULyraCameraMode_FirstPersonADS::OnDeactivation()
{
	SetADSCrosshairVisible(false);
	SetADSFirstPersonMeshesHidden(false);
}

// Owner-only hide: animation, IK, attachments all keep running as default.
// Uses OwnerNoSee ONLY (pure render-view cull, no render-state side effects).
// Ownerless cosmetic part actors are temporarily re-owned to the pawn so the
// flag reaches them too. Original flags/owners are recorded for exact restore.
void ULyraCameraMode_FirstPersonADS::SetADSFirstPersonMeshesHidden(bool bHidden)
{
	ACharacter* TargetCharacter = Cast<ACharacter>(GetTargetActor());
	if (!TargetCharacter || !TargetCharacter->IsLocallyControlled())
	{
		UE_LOG(LogLyra, Warning, TEXT("[FPADS] HideMeshes skipped: no local pawn"));
		return;
	}

	auto HideOneMesh = [this](UMeshComponent* Mesh, const TCHAR* Label)
	{
		if (!Mesh)
		{
			return;
		}
		if (!ADSRestoreStates.Contains(Mesh))
		{
			FADSMeshRestoreState Saved;
			Saved.bOwnerNoSee = Mesh->bOwnerNoSee;
			ADSRestoreStates.Add(Mesh, Saved);
		}
		Mesh->SetOwnerNoSee(true);
		UE_LOG(LogLyra, Display, TEXT("[FPADS]   %s mesh %s -> OwnerNoSee=1"), Label, *Mesh->GetName());
	};

	auto LogAnimState = [](const TCHAR* When, ACharacter* Character)
	{
		if (USkeletalMeshComponent* Body = Character->GetMesh())
		{
			UAnimInstance* Inst = Body->GetAnimInstance();
			const UAnimMontage* Montage = Inst ? Inst->GetCurrentActiveMontage() : nullptr;
			UE_LOG(LogLyra, Display, TEXT("[FPADS] anim %s: body=%s tick=%d tickopt=%d inst=%d montage=%s"),
				When, *Body->GetName(),
				(int32)Body->MeshComponentUpdateFlag,
				(int32)Body->VisibilityBasedAnimTickOption,
				Inst ? 1 : 0,
				Montage ? *Montage->GetName() : TEXT("none"));
		}
	};

	if (bHidden)
	{
		TArray<UMeshComponent*> PawnMeshes;
		TargetCharacter->GetComponents<UMeshComponent>(PawnMeshes);
		UE_LOG(LogLyra, Display, TEXT("[FPADS] HideMeshes=1 pawn=%s meshes=%d"), *TargetCharacter->GetName(), PawnMeshes.Num());
		for (UMeshComponent* Mesh : PawnMeshes)
		{
			HideOneMesh(Mesh, TEXT("pawn"));
		}

		TArray<AActor*> AttachedActors;
		TargetCharacter->GetAttachedActors(AttachedActors, true, true);
		for (AActor* Attached : AttachedActors)
		{
			// OwnerNoSee only hides from the owner's viewer chain. Cosmetic
			// parts spawn ownerless, so lend them the pawn until ADS exit.
			// Replicated actors (weapons) are left alone for net safety.
			if (Attached && Attached->GetOwner() != TargetCharacter && !Attached->GetIsReplicated())
			{
				if (!ADSRestoreOwners.Contains(Attached))
				{
					ADSRestoreOwners.Add(Attached, Attached->GetOwner());
				}
				UE_LOG(LogLyra, Display, TEXT("[FPADS]   re-own %s to pawn"), *Attached->GetName());
				Attached->SetOwner(TargetCharacter);
			}
			TArray<UMeshComponent*> Meshes;
			Attached->GetComponents<UMeshComponent>(Meshes);
			for (UMeshComponent* Mesh : Meshes)
			{
				HideOneMesh(Mesh, *FString::Printf(TEXT("attached %s"), *Attached->GetName()));
			}
		}
		LogAnimState(TEXT("ads-enter"), TargetCharacter);
	}
	else
	{
		UE_LOG(LogLyra, Display, TEXT("[FPADS] HideMeshes=0 pawn=%s restoring %d/%d"), *TargetCharacter->GetName(), ADSRestoreStates.Num(), ADSRestoreOwners.Num());
		LogAnimState(TEXT("ads-exit"), TargetCharacter);
		for (auto& Pair : ADSRestoreStates)
		{
			if (UMeshComponent* Mesh = Pair.Key.Get())
			{
				UE_LOG(LogLyra, Display, TEXT("[FPADS]   restore %s -> OwnerNoSee=%d"), *Mesh->GetName(), Pair.Value.bOwnerNoSee);
				Mesh->SetOwnerNoSee(Pair.Value.bOwnerNoSee);
			}
		}
		ADSRestoreStates.Empty();
		for (auto& Pair : ADSRestoreOwners)
		{
			if (AActor* Attached = Pair.Key.Get())
			{
				AActor* OrigOwner = Pair.Value.Get();
				UE_LOG(LogLyra, Display, TEXT("[FPADS]   restore owner %s -> %s"), *Attached->GetName(), OrigOwner ? *OrigOwner->GetName() : TEXT("none"));
				Attached->SetOwner(OrigOwner);
			}
		}
		ADSRestoreOwners.Empty();
	}
}

void ULyraCameraMode_FirstPersonADS::SetADSCrosshairVisible(bool bVisible) const
{
	const ACharacter* TargetCharacter = Cast<ACharacter>(GetTargetActor());
	if (!TargetCharacter || !TargetCharacter->IsLocallyControlled())
	{
		return;
	}

	ULyraCameraMode_FirstPersonADS* MutableThis = const_cast<ULyraCameraMode_FirstPersonADS*>(this);
	if (bVisible)
	{
		if (!GActiveScopeWidget.IsValid() && MutableThis->CrosshairWidgetClass)
		{
			if (APlayerController* PC = Cast<APlayerController>(TargetCharacter->GetController()))
			{
				if (UUserWidget* Widget = CreateWidget<UUserWidget>(PC, MutableThis->CrosshairWidgetClass))
				{
					Widget->AddToViewport(100);
					GActiveScopeWidget = Widget;
				}
			}
		}
		else if (UUserWidget* Widget = GActiveScopeWidget.Get())
		{
			Widget->SetVisibility(ESlateVisibility::Visible);
		}
	}
	else if (UUserWidget* Widget = GActiveScopeWidget.Get())
	{
		Widget->SetVisibility(ESlateVisibility::Collapsed);
		Widget->RemoveFromParent();
		GActiveScopeWidget = nullptr;
	}
}
