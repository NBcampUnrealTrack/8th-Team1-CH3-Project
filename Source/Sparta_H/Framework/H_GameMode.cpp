// Copyright Epic Games, Inc. All Rights Reserved.

#include "H_GameMode.h"

#include "H_PlayerController.h"
#include "../Characters/PlayerCharacter.h"
#include "UObject/ConstructorHelpers.h"

AH_GameMode::AH_GameMode()
	: Super()
{
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(
		TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	PlayerControllerClass = AH_PlayerController::StaticClass();
}
