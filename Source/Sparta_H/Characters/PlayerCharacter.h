// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../Weapon/WeaponTypes.h"
#include "PlayerCharacter.generated.h"

struct FInputActionValue;
class UCameraComponent;
class UInputAction;
class UCombatManager;
class UWeaponDataAsset;
class AWeaponBase;

UCLASS()
class SPARTA_H_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();

	// 1인칭 카메라 — 캡슐 상단에 부착, 컨트롤러 회전 적용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UCameraComponent* Camera;

protected:
	float MoveSpeed;
	float SprintSpeedMultiplier;
	float SprintSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	bool IsEquipped = false;

public:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION()
	void Move(const FInputActionValue& value);
	UFUNCTION()
	void Look(const FInputActionValue& value);
	UFUNCTION()
	void StartJump(const FInputActionValue& value);
	UFUNCTION()
	void StopJump(const FInputActionValue& value);
	UFUNCTION()
	void StartRun(const FInputActionValue& value);
	UFUNCTION()
	void StopRun(const FInputActionValue& value);
	UFUNCTION()
	void StartHide(const FInputActionValue& value);
	UFUNCTION()
	void StopHide(const FInputActionValue& value);
	UFUNCTION()
	void Roll(const FInputActionValue& value);
	UFUNCTION()
	void StartLean(const FInputActionValue& value);
	UFUNCTION()
	void StopLean(const FInputActionValue& value);
	UFUNCTION()
	void Interaction(const FInputActionValue& value);

	/** Weapon System **/
	UFUNCTION(BlueprintCallable, Category = "Combat")
	UCombatManager* GetCombatManager() const { return CombatManager; }

	// 인덱스로 슬롯 무기 장착. 잘못된 인덱스/같은 무기/재장전·교체 중이면 무시
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeaponByIndex(int32 NewWeaponIndex);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipNextWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipPreviousWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	AWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	UWeaponDataAsset* GetCurrentWeaponData() const;
	
	void ApplyRecoil(const FRecoilData& Recoil);
	/** End of Weapon System **/

	/** 플레이어 스탯 / 목표 — HUD 위젯이 직접 참조 **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats")
	float CurrentHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FString CurrentObjective = TEXT("기밀실로 잠입하여 서류를 획득하십시오.");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|UI")
	ECrosshairState CurrentCrosshairState = ECrosshairState::Default;

	UFUNCTION(BlueprintCallable, Category = "Player|UI")
	void NotifyEnemyKilled();
	/** End of 플레이어 스탯 / 목표 **/

private:
	/** Weapon Input Callbacks **/
	void OnEquipSlotPressed(const FInputActionValue& Value);
	void OnEquipNextPressed(const FInputActionValue& Value);
	void OnEquipPreviousPressed(const FInputActionValue& Value);
	void OnFirePressed(const FInputActionValue& Value);
	void OnReloadPressed(const FInputActionValue& Value);

	// BeginPlay에서 EquippedWeapons 각 DA로 무기 액터를 스폰해 본체 메시 GripPoint에 부착
	void SpawnEquippedWeapons();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatManager> CombatManager;

	// 슬롯에 등록할 무기 DA. BP 디테일에서 인덱스 0~3에 직접 등록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UWeaponDataAsset>> EquippedWeapons;

	// 스폰 시 사용할 베이스 액터 클래스. 무기별 특수 로직 필요해지면 BP에서 파생 클래스 지정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AWeaponBase> WeaponBaseClass;

	// EquippedWeapons와 1:1로 스폰된 무기 액터들. 인덱스 정합 유지를 위해 nullptr도 보존
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<AWeaponBase>> SpawnedWeapons;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AWeaponBase> CurrentWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	int32 CurrentWeaponIndex = 0;
	
	// 회복이 남아있는 누적 pitch (양수 = 위로 튕긴 양)
	float RecoilPitchAccum = 0.0f;
	// 현재 장창 무기의 회복 속도
	float RecoilRecoverySpeed = 0.0f;
};
