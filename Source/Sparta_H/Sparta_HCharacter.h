// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Sparta_HCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

class UWeaponDataAsset;
class AWeaponBase;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

// 1인칭 플레이어 캐릭터. 카메라/팔 메시/무기 슬롯 시스템을 보유
UCLASS(config=Game)
class ASparta_HCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Mesh, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> Mesh1P;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCameraComponent;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	// 1~4 키로 슬롯 직접 선택 (Axis1D, Scalar Modifier 1~4)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> EquipSlotAction;   // Int32 value (1~4)

	// 마우스 휠 다운 — 다음 슬롯으로 순환
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> EquipNextWeaponAction;   // Bool (휠 다운)

	// 마우스 휠 업 — 이전 슬롯으로 순환
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> EquipPreviousWeaponAction; // Bool (휠 업)

public:
	ASparta_HCharacter();

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

protected:
	// APawn interface
	virtual void BeginPlay() override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	/** Weapon System **/
	// 인덱스로 슬롯 무기 장착. 잘못된 인덱스/같은 무기/재장전·교체 중이면 무시
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeaponByIndex(int32 NewWeaponIndex);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipNextWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipPreviousWeapon();

	// 현재 장착된 무기 액터. 발사/반동/탄약 등 무기 측 로직 접근 시 사용
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	AWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

	// 현재 장착된 무기의 DA. AnimBP 분기 등 정적 스펙 조회용
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	UWeaponDataAsset* GetCurrentWeaponData() const;
	/** End of Weapon System **/
	
	/** 플레이어 스탯 / 목표 **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats")
	float CurrentHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FString CurrentObjective = TEXT("기밀실로 잠입하여 서류를 획득하십시오.");
	/** 플레이어 스탯 / 목표 **/

private:
	/** Weapon Input Callbacks **/
	void OnEquipSlotPressed(const FInputActionValue& Value);
	void OnEquipNextPressed(const FInputActionValue& Value);
	void OnEquipPreviousPressed(const FInputActionValue& Value);

	// BeginPlay에서 EquippedWeapons 각 DA로 무기 액터를 스폰해 GripPoint에 부착
	void SpawnEquippedWeapons();

	/** Weapon System **/
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
	/** End of Weapon System **/


};
