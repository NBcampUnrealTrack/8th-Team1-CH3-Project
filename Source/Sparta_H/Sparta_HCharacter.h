// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Weapon/Sparta_HWeaponTypes.h"
#include "Sparta_HCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

class USparta_HWeaponDataAsset;

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

	// 현재 장착된 무기의 DA. 발사/재장전/AnimBP 분기에서 참조
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	USparta_HWeaponDataAsset* GetCurrentWeaponData() const { return CurrentWeaponData; }
	/** End of Weapon System **/

private:
	/** Weapon Input Callbacks **/
	void OnEquipSlotPressed(const FInputActionValue& Value);
	void OnEquipNextPressed(const FInputActionValue& Value);
	void OnEquipPreviousPressed(const FInputActionValue& Value);

	/** Weapon System **/
	// Mesh1P의 GripPoint 소켓에 부착되는 무기 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> WeaponMeshComponent;

	// 인벤토리 슬롯. BP 디테일에서 DA를 인덱스 0~3에 직접 등록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<USparta_HWeaponDataAsset>> EquippedWeapons;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USparta_HWeaponDataAsset> CurrentWeaponData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	int32 CurrentWeaponIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	EWeaponState CurrentWeaponState = EWeaponState::Idle;
	/** End of Weapon System **/
};

