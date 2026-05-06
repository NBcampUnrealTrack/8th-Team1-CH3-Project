#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.generated.h"  

UENUM(BlueprintType)
enum class EHitBone : uint8
{
	Head,
	Torso,
	Arm,
	Leg,
	None
};

UENUM(BlueprintType)
enum class ECombatWeaponType : uint8
{
	Rifle,
	Pistol,
	Knife,
	Grenade, 
	Rock,
	None
};

USTRUCT(BlueprintType)
struct FCombatDamageInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Damage")
	float BaseDamage = 10.0f;
	
	UPROPERTY(EditAnywhere, Category = "Damage")
	float Distance = 0.0f;
	
	UPROPERTY(EditAnywhere, Category = "Damage")
	ECombatWeaponType WeaponType = ECombatWeaponType::None;

	UPROPERTY(EditAnywhere, Category = "Damage")
	EHitBone HitBone = EHitBone::None;
};