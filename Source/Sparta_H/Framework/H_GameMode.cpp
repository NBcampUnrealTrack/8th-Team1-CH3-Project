// Copyright Epic Games, Inc. All Rights Reserved.

#include "H_GameMode.h"

#include "H_PlayerController.h"
#include "../Characters/PlayerCharacter.h"
#include "UObject/ConstructorHelpers.h"

AH_GameMode::AH_GameMode()
	: Super()
{
	DefaultPawnClass = APlayerCharacter::StaticClass();

	PlayerControllerClass = AH_PlayerController::StaticClass();
}
