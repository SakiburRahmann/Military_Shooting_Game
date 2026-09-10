// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LyraCameraMode.h"
#include "LyraCameraMode_FirstPerson.generated.h"

class UUserWidget;
class APlayerController;
class ULyraHealthComponent;
class USkeletalMesh;
class USkeletalMeshComponent;

/**
 * ULyraCameraMode_FirstPerson
 *
 *	True first-person camera anchored to the animated head bone, so it tracks
 *	run/sprint animation instead of floating on capsule math. Eyes sit ~12cm in
 *	front of and ~5cm above the head joint, in front of the face plane, which
 *	also removes any need to hide the head.
 */
UCLASS(Blueprintable)
class ULyraCameraMode_FirstPerson : public ULyraCameraMode
{
	GENERATED_BODY()

public:

	ULyraCameraMode_FirstPerson();

	float GetEyeForwardOffset() const { return EyeForwardOffset; }
	float GetEyeUpOffset() const { return EyeUpOffset; }
	float GetHipFieldOfView() const { return FieldOfView; }

protected:

	virtual void UpdateView(float DeltaTime) override;
	void PreventHeadPenetration(const AActor* TargetActor, const FVector& SafeLoc, FVector& CameraLoc) const;
	bool IsTargetDeadOrDying(const ACharacter* TargetCharacter) const;

protected:

	// Head bone the camera tracks.
	UPROPERTY(EditDefaultsOnly, Category = "First Person")
	FName HeadBoneName = TEXT("head");

	// Eye offset from the head joint, applied along the look direction.
	UPROPERTY(EditDefaultsOnly, Category = "First Person", Meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "30.0"))
	float EyeForwardOffset = 12.0f;

	// Eye offset above the head joint.
	UPROPERTY(EditDefaultsOnly, Category = "First Person", Meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "30.0"))
	float EyeUpOffset = 5.0f;

	// Wall-clip probe radius.
	UPROPERTY(EditDefaultsOnly, Category = "First Person", Meta = (ClampMin = "1.0", UIMin = "1.0", UIMax = "20.0"))
	float PenetrationProbeRadius = 8.0f;

private:

	mutable TWeakObjectPtr<const USkeletalMeshComponent> CachedHeadMesh;
	mutable TWeakObjectPtr<const USkeletalMesh> CachedHeadSkeletalMesh;
	mutable int32 CachedHeadBoneIndex = INDEX_NONE;
	mutable FName CachedHeadBoneName;
	mutable TWeakObjectPtr<const ULyraHealthComponent> CachedHealthComp;
	mutable TWeakObjectPtr<const ACharacter> CachedHealthPawn;
};


/**
 * ULyraCameraMode_FirstPersonADS
 *
 *	Aim-down-sights variant: narrower FOV, camera pushed toward the sights,
 *	faster blend. Pushed by the ADS ability in place of the third-person ADS mode.
 */
UCLASS(Blueprintable)
class ULyraCameraMode_FirstPersonADS : public ULyraCameraMode_FirstPerson
{
	GENERATED_BODY()

public:

	ULyraCameraMode_FirstPersonADS();

protected:

	virtual void UpdateView(float DeltaTime) override;
	virtual void OnActivation() override;
	virtual void OnDeactivation() override;
	virtual void BeginDestroy() override;

	void SetADSCrosshairVisible(bool bVisible);
	void SetADSFirstPersonMeshesHidden(bool bHidden);
	void RestoreADSState();
	void CleanupScopeWidget();
	bool ValidateScopeWidget(APlayerController* PC) const;
	void HideNewPawnMeshes(ACharacter* TargetCharacter);

protected:

	UPROPERTY(EditDefaultsOnly, Category = "First Person ADS")
	TSubclassOf<UUserWidget> CrosshairWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveScopeWidget = nullptr;

	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerController> OwningPC;

	UPROPERTY(EditDefaultsOnly, Category = "First Person ADS")
	float HipFieldOfView = 90.0f;

	bool bMeshesHidden = false;

private:

	// Original OwnerNoSee each mesh had before ADS hid it, so ADS exit
	// restores the authored state instead of forcing everything visible
	// (Lyra intentionally keeps some meshes owner-hidden).
	struct FADSMeshRestoreState
	{
		bool bOwnerNoSee = false;
	};
	TMap<TWeakObjectPtr<class UMeshComponent>, FADSMeshRestoreState> ADSRestoreStates;
	// Original owners of attached actors we re-owned to the pawn (lets
	// OwnerNoSee reach ownerless cosmetic parts without touching HiddenInGame).
	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<AActor>> ADSRestoreOwners;
};
