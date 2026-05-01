// Copyright Epic Games, Inc. All Rights Reserved.

#include "Sparta_HGameMode.h"
#include "Sparta_HCharacter.h"
#include "UObject/ConstructorHelpers.h"

ASparta_HGameMode::ASparta_HGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
