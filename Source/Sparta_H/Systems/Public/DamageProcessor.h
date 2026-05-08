#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CombatTypes.h"
#include "DamageProcessor.generated.h"

/**
 * 
 */
UCLASS()
class SPARTA_H_API UDamageProcessor : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Damage")
	float BoneHead = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Damage")
	float BoneTorso = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Damage")
	float BoneLimb = 0.7f;

	float CalculateFinalDamage(const FCombatDamageInfo& Info);
	float GetDistanceMultiplier(float Distance, ECombatWeaponType WeaponType);
	float GetBoneMultiplier(EHitBone Bone);

private:
	static constexpr float GUN_RANGE_T1 = 5000.f; // 50m
	static constexpr float GUN_RANGE_T2 = 10000.f; // 100m
	static constexpr float GUN_RANGE_T3 = 15000.f; // 150m
	static constexpr float GUN_RANGE_T4 = 20000.f; // 200m

	static constexpr float GUN_MULT_T1 = 1.0f;
	static constexpr float GUN_MULT_T2 = 0.7f;
	static constexpr float GUN_MULT_T3 = 0.5f;
	static constexpr float GUN_MULT_T4 = 0.3f;
	static constexpr float GUN_MULT_T5 = 0.0f;
};
