#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnemyStatDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class SPARTA_H_API UEnemyStatDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	// 기획서: Normal 100 / Elite 150
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float MaxHealth = 100.f;
 
	// 기획서: Normal 15 / Elite 20
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float Damage = 15.f;
	
};
