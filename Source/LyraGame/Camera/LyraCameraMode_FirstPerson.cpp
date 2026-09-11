// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraCameraMode_FirstPerson.h"
#include "Camera/LyraCameraMode.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Character/LyraHealthComponent.h"
#include "Cosmetics/LyraSoldierPartActor.h"
#include "Engine/World.h"
#include "Engine/HitResult.h"
#include "CollisionQueryParams.h"
#include "LyraLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraCameraMode_FirstPerson)

static TMap<TWeakObjectPtr<APlayerController>, TWeakObjectPtr<UUserWidget>> GScopeWidgetsByPC;

struct FFPHeadPawnState
{
	TMap<TWeakObjectPtr<UMeshComponent>, bool> Meshes;
	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<AActor>> Owners;
	int32 RefCount = 0;
};

static TMap<TWeakObjectPtr<AActor>, FFPHeadPawnState> GFPHeadStates;

static void FPHeadPrune()
{
	for (auto It = GFPHeadStates.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

static bool IsHeadGearActor(AActor* PartActor)
{
	if (const ALyraSoldierPartActor* SoldierPart = Cast<ALyraSoldierPartActor>(PartActor))
	{
		if (SoldierPart->PartSlot == ESoldierPartSlot::Head || SoldierPart->PartSlot == ESoldierPartSlot::Helmet)
		{
			return true;
		}
	}
	const FString ClassName = PartActor->GetClass()->GetName();
	return ClassName.Contains(TEXT("Helmet")) || ClassName.Contains(TEXT("Head"));
}

static void FPHeadApply(ACharacter* Pawn, FFPHeadPawnState& State)
{
	TArray<AActor*> AttachedActors;
	Pawn->GetAttachedActors(AttachedActors, true, true);
	for (AActor* PartActor : AttachedActors)
	{
		if (!PartActor || PartActor == Pawn || !IsHeadGearActor(PartActor))
		{
			continue;
		}
		if (PartActor->GetOwner() != Pawn && !PartActor->GetIsReplicated())
		{
			if (!State.Owners.Contains(PartActor))
			{
				State.Owners.Add(PartActor, PartActor->GetOwner());
			}
			PartActor->SetOwner(Pawn);
		}
		TArray<UMeshComponent*> Meshes;
		PartActor->GetComponents<UMeshComponent>(Meshes);
		for (UMeshComponent* Mesh : Meshes)
		{
			if (Mesh && !State.Meshes.Contains(Mesh))
			{
				State.Meshes.Add(Mesh, Mesh->bOwnerNoSee ? true : false);
			}
			if (Mesh)
			{
				Mesh->SetOwnerNoSee(true);
			}
		}
	}
}

static void FPHeadUnapply(ACharacter* Pawn, FFPHeadPawnState& State)
{
	for (auto& Pair : State.Meshes)
	{
		if (UMeshComponent* Mesh = Pair.Key.Get())
		{
			if (IsValid(Mesh))
			{
				Mesh->SetOwnerNoSee(Pair.Value);
			}
		}
	}
	State.Meshes.Empty();
	for (auto& Pair : State.Owners)
	{
		AActor* PartActor = Pair.Key.Get();
		AActor* OrigOwner = Pair.Value.Get();
		if (IsValid(PartActor) && PartActor->GetOwner() == Pawn)
		{
			PartActor->SetOwner(OrigOwner);
		}
	}
	State.Owners.Empty();
}

static void PruneScopeWidgetMap()
{
	check(IsInGameThread());
	for (auto It = GScopeWidgetsByPC.CreateIterator(); It; ++It)
	{
		UUserWidget* Widget = It->Value.Get();
		if (!It->Key.IsValid() || !IsValid(Widget) || !Widget->IsInViewport())
		{
			It.RemoveCurrent();
		}
	}
}

ULyraCameraMode_FirstPerson::ULyraCameraMode_FirstPerson()
{
	FieldOfView = 90.0f;
	BlendTime = 0.1f;
}

bool ULyraCameraMode_FirstPerson::IsTargetDeadOrDying(const ACharacter* TargetCharacter) const
{
	if (!TargetCharacter)
	{
		return false;
	}
	if (CachedHealthPawn != TargetCharacter || !CachedHealthComp.IsValid())
	{
		CachedHealthPawn = TargetCharacter;
		CachedHealthComp = TargetCharacter->FindComponentByClass<ULyraHealthComponent>();
	}
	const ULyraHealthComponent* HealthComp = CachedHealthComp.Get();
	return HealthComp && HealthComp->IsDeadOrDying();
}

void ULyraCameraMode_FirstPerson::UpdateView(float DeltaTime)
{
	AActor* TargetActor = GetTargetActor();
	if (!TargetActor)
	{
		return;
	}

	if (FPHeadHideTarget != TargetActor)
	{
		RestoreFPHead();
		FPHeadHideTarget = TargetActor;
	}

	FVector PivotLocation = FVector::ZeroVector;
	bool bFoundHead = false;

	if (const ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
	{
		if (!IsTargetDeadOrDying(TargetCharacter))
		{
			if (const USkeletalMeshComponent* Mesh = TargetCharacter->GetMesh())
			{
				const USkeletalMesh* SkelMesh = Mesh->GetSkeletalMeshAsset();
				if (CachedHeadMesh != Mesh || CachedHeadBoneName != HeadBoneName || CachedHeadSkeletalMesh != SkelMesh)
				{
					CachedHeadMesh = Mesh;
					CachedHeadSkeletalMesh = SkelMesh;
					CachedHeadBoneName = HeadBoneName;
					CachedHeadBoneIndex = Mesh->GetBoneIndex(HeadBoneName);
				}
				if (CachedHeadBoneIndex != INDEX_NONE)
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

	const FQuat PivotQuat(PivotRotation);
	const FVector EyeLocation = PivotLocation
		+ PivotQuat.GetForwardVector() * EyeForwardOffset
		+ PivotQuat.GetUpVector() * EyeUpOffset;

	FVector DesiredLocation = EyeLocation;
	PreventHeadPenetration(TargetActor, PivotLocation, DesiredLocation);

	View.Location = DesiredLocation;
	View.Rotation = PivotRotation;
	View.ControlRotation = View.Rotation;
	View.FieldOfView = FieldOfView;

	if (ACharacter* FPCharacter = Cast<ACharacter>(TargetActor))
	{
		if (FPCharacter->IsLocallyControlled() && !IsTargetDeadOrDying(FPCharacter))
		{
			EnsureFPHeadHidden(FPCharacter);
		}
		else
		{
			RestoreFPHead();
		}
	}
}

void ULyraCameraMode_FirstPerson::OnDeactivation()
{
	RestoreFPHead();
	FPHeadHideTarget = nullptr;
}

void ULyraCameraMode_FirstPerson::BeginDestroy()
{
	RestoreFPHead();
	FPHeadHideTarget = nullptr;
	Super::BeginDestroy();
}

void ULyraCameraMode_FirstPerson::EnsureFPHeadHidden(ACharacter* TargetCharacter)
{
	if (!TargetCharacter)
	{
		return;
	}
	FPHeadPrune();
	if (bFPHeadAcquired && !GFPHeadStates.Contains(TargetCharacter))
	{
		bFPHeadAcquired = false;
	}
	if (bFPHeadAcquired)
	{
		return;
	}
	FFPHeadPawnState& State = GFPHeadStates.FindOrAdd(TargetCharacter);
	if (State.RefCount == 0)
	{
		FPHeadApply(TargetCharacter, State);
	}
	State.RefCount++;
	bFPHeadAcquired = true;
}

void ULyraCameraMode_FirstPerson::RestoreFPHead()
{
	if (!bFPHeadAcquired)
	{
		return;
	}
	bFPHeadAcquired = false;
	FPHeadPrune();
	AActor* Pawn = FPHeadHideTarget.Get();
	if (!IsValid(Pawn))
	{
		return;
	}
	if (FFPHeadPawnState* State = GFPHeadStates.Find(Pawn))
	{
		if (--State->RefCount <= 0)
		{
			if (ACharacter* Char = Cast<ACharacter>(Pawn))
			{
				FPHeadUnapply(Char, *State);
			}
			GFPHeadStates.Remove(Pawn);
		}
	}
}

void ULyraCameraMode_FirstPerson::ForceRestoreFPHeadForPawn(AActor* Pawn)
{
	if (!IsValid(Pawn))
	{
		return;
	}
	FPHeadPrune();
	GFPHeadStates.Remove(Pawn);
	ACharacter* Char = Cast<ACharacter>(Pawn);
	if (!Char)
	{
		return;
	}
	TArray<AActor*> AttachedActors;
	Char->GetAttachedActors(AttachedActors, true, true);
	for (AActor* PartActor : AttachedActors)
	{
		if (!IsValid(PartActor) || PartActor == Pawn || !IsHeadGearActor(PartActor))
		{
			continue;
		}
		if (PartActor->GetOwner() == Pawn)
		{
			PartActor->SetOwner(nullptr);
		}
		TArray<UMeshComponent*> Meshes;
		PartActor->GetComponents<UMeshComponent>(Meshes);
		for (UMeshComponent* Mesh : Meshes)
		{
			if (IsValid(Mesh))
			{
				Mesh->SetOwnerNoSee(false);
			}
		}
	}
}

void ULyraCameraMode_FirstPerson::PreventHeadPenetration(const AActor* TargetActor, const FVector& SafeLoc, FVector& CameraLoc) const
{
	UWorld* World = GetWorld();
	if (!World || !TargetActor)
	{
		return;
	}

	const float Radius = FMath::Max(PenetrationProbeRadius, 1.0f);
	const FVector Delta = CameraLoc - SafeLoc;
	if (Delta.IsNearlyZero())
	{
		return;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(FPCameraPenetration), false, TargetActor);
	if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		TArray<AActor*> Attached;
		TargetPawn->GetAttachedActors(Attached, true, true);
		Params.AddIgnoredActors(Attached);
	}

	FHitResult Hit;
	if (World->SweepSingleByChannel(Hit, SafeLoc, CameraLoc, FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(Radius), Params))
	{
		if (Hit.bStartPenetrating)
		{
			CameraLoc = SafeLoc;
			return;
		}
		FVector PulledBack = Hit.Location + (Hit.Normal * Radius);
		if (FVector::DotProduct(PulledBack - SafeLoc, Delta) < 0.f)
		{
			PulledBack = SafeLoc;
		}
		CameraLoc = PulledBack;
	}
}

ULyraCameraMode_FirstPersonADS::ULyraCameraMode_FirstPersonADS()
{
	FieldOfView = 60.0f;
	BlendTime = 0.15f;
	BlendFunction = ELyraCameraModeBlendFunction::EaseInOut;

	if (const ULyraCameraMode_FirstPerson* DefaultFP = GetDefault<ULyraCameraMode_FirstPerson>())
	{
		HipFieldOfView = DefaultFP->GetHipFieldOfView();
		EyeForwardOffset = DefaultFP->GetEyeForwardOffset();
		EyeUpOffset = DefaultFP->GetEyeUpOffset();
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> CrosshairFinder(TEXT("/Game/UI/W_ADSCrosshair"));
	if (CrosshairFinder.Succeeded())
	{
		CrosshairWidgetClass = CrosshairFinder.Class;
	}
	else
	{
		static bool bWarnedMissingScope = false;
		if (!bWarnedMissingScope)
		{
			bWarnedMissingScope = true;
			UE_LOG(LogLyra, Warning, TEXT("[FPADS] /Game/UI/W_ADSCrosshair not found; scope overlay disabled"));
		}
	}
}

void ULyraCameraMode_FirstPersonADS::UpdateView(float DeltaTime)
{
	AActor* TargetActor = GetTargetActor();
	if (!TargetActor)
	{
		return;
	}

	if (const ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
	{
		if (IsTargetDeadOrDying(TargetCharacter))
		{
			if (bMeshesHidden || ActiveScopeWidget)
			{
				SetADSCrosshairVisible(false);
				SetADSFirstPersonMeshesHidden(false);
			}
			ULyraCameraMode_FirstPerson::UpdateView(DeltaTime);
			View.FieldOfView = HipFieldOfView;
			return;
		}
	}

	ULyraCameraMode_FirstPerson::UpdateView(DeltaTime);
	View.FieldOfView = FieldOfView;

	if (!bMeshesHidden)
	{
		SetADSFirstPersonMeshesHidden(true);
	}
	else if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
	{
		HideNewPawnMeshes(TargetCharacter);
	}
}

void ULyraCameraMode_FirstPersonADS::HideNewPawnMeshes(ACharacter* TargetCharacter)
{
	if (!TargetCharacter)
	{
		return;
	}
	TArray<UMeshComponent*> PawnMeshes;
	TargetCharacter->GetComponents<UMeshComponent>(PawnMeshes);
	for (UMeshComponent* Mesh : PawnMeshes)
	{
		if (Mesh && !ADSRestoreStates.Contains(Mesh))
		{
			FADSMeshRestoreState Saved;
			Saved.bOwnerNoSee = Mesh->bOwnerNoSee ? true : false;
			ADSRestoreStates.Add(Mesh, Saved);
			Mesh->SetOwnerNoSee(true);
		}
	}
	TArray<AActor*> AttachedActors;
	TargetCharacter->GetAttachedActors(AttachedActors, true, true);
	for (AActor* Attached : AttachedActors)
	{
		if (!Attached || IsHeadGearActor(Attached))
		{
			continue;
		}
		TArray<UMeshComponent*> Meshes;
		Attached->GetComponents<UMeshComponent>(Meshes);
		for (UMeshComponent* Mesh : Meshes)
		{
			if (Mesh && !ADSRestoreStates.Contains(Mesh))
			{
				FADSMeshRestoreState Saved;
				Saved.bOwnerNoSee = Mesh->bOwnerNoSee ? true : false;
				ADSRestoreStates.Add(Mesh, Saved);
				Mesh->SetOwnerNoSee(true);
			}
		}
	}
}

void ULyraCameraMode_FirstPersonADS::OnActivation()
{
	if (const ULyraCameraMode_FirstPerson* DefaultFP = GetDefault<ULyraCameraMode_FirstPerson>())
	{
		HipFieldOfView = DefaultFP->GetHipFieldOfView();
	}
	if (!CrosshairWidgetClass)
	{
		static bool bWarnedMissingClass = false;
		if (!bWarnedMissingClass)
		{
			bWarnedMissingClass = true;
			UE_LOG(LogLyra, Warning, TEXT("[FPADS] Missing CrosshairWidgetClass; scope overlay disabled"));
		}
	}
	PruneScopeWidgetMap();
	SetADSCrosshairVisible(true);
	SetADSFirstPersonMeshesHidden(true);
}

void ULyraCameraMode_FirstPersonADS::OnDeactivation()
{
	SetADSCrosshairVisible(false);
	SetADSFirstPersonMeshesHidden(false);
	RestoreFPHead();
}

void ULyraCameraMode_FirstPersonADS::BeginDestroy()
{
	if (ActiveScopeWidget || bMeshesHidden)
	{
		if (UUserWidget* Widget = ActiveScopeWidget)
		{
			if (IsValid(Widget))
			{
				Widget->SetVisibility(ESlateVisibility::Collapsed);
				if (Widget->IsInViewport())
				{
					Widget->RemoveFromParent();
				}
			}
		}
		ActiveScopeWidget = nullptr;
		RestoreADSState();
		PruneScopeWidgetMap();
	}
	Super::BeginDestroy();
}

void ULyraCameraMode_FirstPersonADS::RestoreADSState()
{
	for (auto& Pair : ADSRestoreStates)
	{
		UMeshComponent* Mesh = Pair.Key.Get();
		if (IsValid(Mesh) && Mesh->bOwnerNoSee)
		{
			Mesh->SetOwnerNoSee(Pair.Value.bOwnerNoSee);
		}
	}
	ADSRestoreStates.Empty();
	for (auto& Pair : ADSRestoreOwners)
	{
		AActor* Attached = Pair.Key.Get();
		AActor* OrigOwner = Pair.Value.Get();
		if (IsValid(Attached) && Attached->GetOwner() == GetTargetActor())
		{
			Attached->SetOwner(OrigOwner);
		}
	}
	ADSRestoreOwners.Empty();
}

void ULyraCameraMode_FirstPersonADS::SetADSFirstPersonMeshesHidden(bool bHidden)
{
	if (bHidden == bMeshesHidden)
	{
		return;
	}

	// Restore must always run (possession loss, death, remote view) even when
	// we are no longer allowed to apply hiding.
	if (!bHidden)
	{
		bMeshesHidden = false;
		RestoreADSState();
		PruneScopeWidgetMap();
		return;
	}

	ACharacter* TargetCharacter = Cast<ACharacter>(GetTargetActor());
	if (!TargetCharacter || !TargetCharacter->IsLocallyControlled())
	{
		return;
	}

	bMeshesHidden = true;

	TArray<UMeshComponent*> PawnMeshes;
	TargetCharacter->GetComponents<UMeshComponent>(PawnMeshes);
	for (UMeshComponent* Mesh : PawnMeshes)
	{
		if (Mesh && !ADSRestoreStates.Contains(Mesh))
		{
			FADSMeshRestoreState Saved;
			Saved.bOwnerNoSee = Mesh->bOwnerNoSee ? true : false;
			ADSRestoreStates.Add(Mesh, Saved);
		}
		if (Mesh)
		{
			Mesh->SetOwnerNoSee(true);
		}
	}

	TArray<AActor*> AttachedActors;
	TargetCharacter->GetAttachedActors(AttachedActors, true, true);
	for (AActor* Attached : AttachedActors)
	{
		if (!Attached || IsHeadGearActor(Attached))
		{
			continue;
		}
		const bool bAuthority = TargetCharacter->HasAuthority();
		if (Attached->GetOwner() != TargetCharacter && !Attached->GetIsReplicated() && bAuthority)
		{
			if (!ADSRestoreOwners.Contains(Attached))
			{
				ADSRestoreOwners.Add(Attached, Attached->GetOwner());
			}
			Attached->SetOwner(TargetCharacter);
		}
		TArray<UMeshComponent*> Meshes;
		Attached->GetComponents<UMeshComponent>(Meshes);
		for (UMeshComponent* Mesh : Meshes)
		{
			if (Mesh && !ADSRestoreStates.Contains(Mesh))
			{
				FADSMeshRestoreState Saved;
				Saved.bOwnerNoSee = Mesh->bOwnerNoSee ? true : false;
				ADSRestoreStates.Add(Mesh, Saved);
			}
			if (Mesh)
			{
				Mesh->SetOwnerNoSee(true);
			}
		}
	}
}

bool ULyraCameraMode_FirstPersonADS::ValidateScopeWidget(APlayerController* PC) const
{
	const UUserWidget* Widget = ActiveScopeWidget;
	if (!IsValid(Widget))
	{
		return false;
	}
	if (Widget->GetOwningPlayer() != PC)
	{
		return false;
	}
	if (PC && Widget->GetWorld() != PC->GetWorld())
	{
		return false;
	}
	return true;
}

void ULyraCameraMode_FirstPersonADS::SetADSCrosshairVisible(bool bVisible)
{
	const ACharacter* TargetCharacter = Cast<ACharacter>(GetTargetActor());
	if (!TargetCharacter || !TargetCharacter->IsLocallyControlled())
	{
		if (!bVisible)
		{
			CleanupScopeWidget();
		}
		return;
	}

	APlayerController* PC = Cast<APlayerController>(TargetCharacter->GetController());
	if (!PC)
	{
		if (!bVisible)
		{
			CleanupScopeWidget();
		}
		return;
	}

	if (bVisible)
	{
		PruneScopeWidgetMap();
		if (!ValidateScopeWidget(PC))
		{
			ActiveScopeWidget = nullptr;
			if (TWeakObjectPtr<UUserWidget>* Found = GScopeWidgetsByPC.Find(PC))
			{
				UUserWidget* Candidate = Found->Get();
				if (IsValid(Candidate) && Candidate->GetOwningPlayer() == PC && Candidate->GetWorld() == PC->GetWorld())
				{
					ActiveScopeWidget = Candidate;
				}
				else
				{
					GScopeWidgetsByPC.Remove(PC);
				}
			}
		}

		if (!ActiveScopeWidget && CrosshairWidgetClass)
		{
			if (UUserWidget* Widget = CreateWidget<UUserWidget>(PC, CrosshairWidgetClass))
			{
				Widget->AddToViewport(100);
				ActiveScopeWidget = Widget;
				OwningPC = PC;
				GScopeWidgetsByPC.Add(PC, Widget);
			}
		}
		else if (ActiveScopeWidget)
		{
			ActiveScopeWidget->SetVisibility(ESlateVisibility::Visible);
			OwningPC = PC;
			GScopeWidgetsByPC.Add(PC, ActiveScopeWidget);
		}
	}
	else
	{
		CleanupScopeWidget();
	}
}

void ULyraCameraMode_FirstPersonADS::CleanupScopeWidget()
{
	if (UUserWidget* Widget = ActiveScopeWidget)
	{
		if (IsValid(Widget))
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
			if (Widget->IsInViewport())
			{
				Widget->RemoveFromParent();
			}
		}
	}
	ActiveScopeWidget = nullptr;

	if (APlayerController* PC = OwningPC.Get())
	{
		GScopeWidgetsByPC.Remove(PC);
	}
	OwningPC = nullptr;
	PruneScopeWidgetMap();
}
