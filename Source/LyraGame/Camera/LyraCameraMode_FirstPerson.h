// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LyraCameraMode.h"
#include "LyraCameraMode_FirstPerson.generated.h"

class UUserWidget;

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

protected:

	virtual void OnActivation() override;
	virtual void UpdateView(float DeltaTime) override;

protected:

	// Head bone the camera tracks.
	UPROPERTY(EditDefaultsOnly, Category = "First Person")
	FName HeadBoneName = TEXT("head");

	// Eye offset from the head joint, applied along the look direction.
	UPROPERTY(EditDefaultsOnly, Category = "First Person")
	float EyeForwardOffset = 12.0f;

	// Eye offset above the head joint.
	UPROPERTY(EditDefaultsOnly, Category = "First Person")
	float EyeUpOffset = 5.0f;
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

	// Shows/hides the ADS crosshair overlay.
	void SetADSCrosshairVisible(bool bVisible) const;
	// Hides the owner's body + held weapon (owner-only, others still see you).
	void SetADSFirstPersonMeshesHidden(bool bHidden);

protected:

	// Crosshair overlay widget shown while aiming.
	UPROPERTY(EditDefaultsOnly, Category = "First Person ADS")
	TSubclassOf<UUserWidget> CrosshairWidgetClass;

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
