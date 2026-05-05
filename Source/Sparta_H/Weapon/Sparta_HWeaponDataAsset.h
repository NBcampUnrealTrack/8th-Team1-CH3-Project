#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Sparta_HWeaponTypes.h"
#include "Sparta_HWeaponDataAsset.generated.h"

UCLASS(BlueprintType)
class SPARTA_H_API USparta_HWeaponDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EWeaponType WeaponType = EWeaponType::Pistol;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EWeaponFireMode FireMode = EWeaponFireMode::SemiAuto;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float Damage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float BackAttackDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float SoundRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float FireRate = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 MaxAmmoCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float ReloadTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FRecoilData RecoilData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bIsReloadable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bShouldAutoReload = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bShouldTriggerAIAggro = false;
};
