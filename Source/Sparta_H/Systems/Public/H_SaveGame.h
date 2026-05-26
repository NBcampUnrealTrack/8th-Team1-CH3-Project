#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "H_SaveGame.generated.h"

UCLASS()
class SPARTA_H_API UH_SaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData")
	int32 CurrentMission = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData")
	bool bHasRifle = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData")
	bool bHasBossKey = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData")
	FVector CurrentLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData")
	FRotator CurrentRotation = FRotator::ZeroRotator;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData")
	int32 KillCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData")
	float SavedElapsedTime = 0.0f;
};
