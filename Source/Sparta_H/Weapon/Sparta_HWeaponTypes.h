#pragma once

#include "CoreMinimal.h"
#include "Sparta_HWeaponTypes.generated.h"

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	Knife,
	Pistol,
	Rifle,
	Rock
};

UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
	Melee,
	SemiAuto,
	FullAuto,
	Throwable
};

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	Idle,
	Firing,
	Reloading,
	Swapping
};

USTRUCT(BlueprintType)
struct FRecoilData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil")
	float VerticalRecoil = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil")
	float HorizontalRecoil = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil")
	float RecoverySpeed = 0.0f;
};

USTRUCT(BlueprintType)
struct FWeaponSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EWeaponType WeaponType = EWeaponType::Pistol;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EWeaponState WeaponState = EWeaponState::Idle;
};
