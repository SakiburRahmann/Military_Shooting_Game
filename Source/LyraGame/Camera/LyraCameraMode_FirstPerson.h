// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LyraCameraMode.h"
#include "LyraCameraMode_FirstPerson.generated.h"

/**
 * ULyraCameraMode_FirstPerson
 *
 * A basic first person camera mode for Lyra.
 */
UCLASS(Blueprintable, BlueprintType)
class ULyraCameraMode_FirstPerson : public ULyraCameraMode
{
	GENERATED_BODY()

public:

	ULyraCameraMode_FirstPerson();

protected:

	virtual void UpdateView(float DeltaTime) override;

	virtual void OnActivation() override;
	virtual void OnDeactivation() override;
};
