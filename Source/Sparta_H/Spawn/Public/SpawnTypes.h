#pragma once

#include "CoreMinimal.h"
#include "SpawnTypes.generated.h"

UENUM()
enum class ESpawnState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Triggered UMETA(DisplayName = "Triggered"),
	CombatSpawn UMETA(DisplayName = "CombatSpawn"),
	Completed UMETA(DisplayName = "Completed"),
};
