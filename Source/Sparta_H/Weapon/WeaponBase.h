#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "WeaponBase.generated.h"

class USkeletalMeshComponent;
class UAmmoComponent;
class UWeaponDataAsset;

// 모든 무기의 베이스 액터. DA를 받아 메시/탄약을 초기화하고,
// 캐릭터의 GripPoint 소켓에 부착되어 Show/Hide로 슬롯 상태를 표현
UCLASS()
class SPARTA_H_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();

	// 스폰 직후 캐릭터가 호출. DA를 보관하고 메시/애님/탄약을 세팅
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Initialize(UWeaponDataAsset* InWeaponData);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	UWeaponDataAsset* GetWeaponData() const { return WeaponData; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMeshComponent; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	UAmmoComponent* GetAmmoComponent() const { return AmmoComponent; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	EWeaponState GetWeaponState() const { return CurrentWeaponState; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetWeaponState(EWeaponState NewState) { CurrentWeaponState = NewState; }

private:
	// 메시 자체를 루트로 — 캐릭터 GripPoint에 SnapToTarget으로 부착하면 메시가 소켓 트랜스폼에 맞춰짐
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> WeaponMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAmmoComponent> AmmoComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponDataAsset> WeaponData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	EWeaponState CurrentWeaponState = EWeaponState::Idle;
};
