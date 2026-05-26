// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WeaponTypes.h"
#include "Systems/Public/CombatTypes.h"
#include "PlayerCharacter.generated.h"

// Forward declarations

class USpringArmComponent;
class UCameraComponent;
class UAnimMontage;
class UCombatManager;
class UWeaponDataAsset;
class AWeaponBase;
class UMissionDataAsset;
class UHealthComponent;
class UStaminaComponent;
class UInteractionComponent;
class UNoiseComponent;
class UVisibilityComponent;
class UDamageType;
class APlayerCharacter;

// ───── Delegates ─────

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObjectiveChanged, const FString&, NewObjective);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMissionStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCrosshairStateChangedDelegate, ECrosshairState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerTakeDamage, float, DamageAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatChanged, float, CurrentValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponChanged, class APlayerCharacter*, Character);

// ───── Structs ─────

USTRUCT(BlueprintType)
struct FPlayerCheckpointData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MissionIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasRifle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasBossKey = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 KillCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ElapsedTime = 0.0f;
};

// ───── Class Definition ─────

UCLASS()
class SPARTA_H_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();

	// ─── Status & Checkpoint ───
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	bool bHasRifle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	bool bHasBossKey = false;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsRifle() const {return bHasRifle;}
	
	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	FPlayerCheckpointData SaveCheckpoint();

	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	void LoadCheckpoint(const FPlayerCheckpointData& CheckpointData);

	// ─── UI Events (Delegates) ───

	// Modified: 각 스탯별 전용 UI 통보 델리게이트 (데이터 혼선 방지)
	UPROPERTY(BlueprintAssignable, Category = "Player|UI")
	FOnStatChanged OnHealthChangedUI;

	UPROPERTY(BlueprintAssignable, Category = "Player|UI")
	FOnStatChanged OnStaminaChangedUI;

	UPROPERTY(BlueprintAssignable, Category = "Player|UI")
	FOnStatChanged OnNoiseChangedUI;

	UPROPERTY(BlueprintAssignable, Category = "Player|UI")
	FOnWeaponChanged OnWeaponChanged;

	UPROPERTY(BlueprintAssignable, Category = "Objective")
	FOnObjectiveChanged OnObjectiveChanged;

	UPROPERTY(BlueprintAssignable, Category = "Player|UI")
	FOnCrosshairStateChangedDelegate OnCrosshairStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Player|Effects")
	FOnPlayerTakeDamage OnPlayerTakeDamage;

	UPROPERTY(BlueprintAssignable, Category = "Objective")
	FOnMissionStateChanged OnMissionCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Objective")
	FOnMissionStateChanged OnMissionFailed;

	// ─── Components (Public Accessors) ───

	UHealthComponent* GetHealthComponent() const { return HealthComponent; }
	UStaminaComponent* GetStaminaComponent() const { return StaminaComponent; }
	UInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }
	UNoiseComponent* GetNoiseComponent() const { return NoiseComponent; }
	UVisibilityComponent* GetVisibilityComponent() const { return VisibilityComponent; }
	UCombatManager* GetCombatManager() const { return CombatManager; }

	// ─── Overrides ───

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Jump() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// ─── Input Handlers ───

	UFUNCTION() void Move(const struct FInputActionValue& value);
	UFUNCTION() void Look(const struct FInputActionValue& value);
	UFUNCTION() void StartJump(const struct FInputActionValue& value);
	UFUNCTION() void StopJump(const struct FInputActionValue& value);
	UFUNCTION() void StartRun(const struct FInputActionValue& value);
	UFUNCTION() void StopRun(const struct FInputActionValue& value);
	UFUNCTION() void StartHide(const struct FInputActionValue& value);
	UFUNCTION() void StopHide(const struct FInputActionValue& value);
	UFUNCTION() void Roll(const struct FInputActionValue& value);
	UFUNCTION() void StartLeanRight(const struct FInputActionValue& value);
	UFUNCTION() void StartLeanLeft(const struct FInputActionValue& value);
	UFUNCTION() void StopLean(const struct FInputActionValue& value);
	UFUNCTION() void Interaction(const struct FInputActionValue& value);

	// ─── Weapon System ───

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	TArray<UWeaponDataAsset*> GetEquippedWeapons() const { return EquippedWeapons; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeaponByIndex(int32 NewWeaponIndex);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipNextWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipPreviousWeapon();

	bool CanEquipSlot(int32 Index) const;
	void CycleWeapon(int32 Direction);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	AWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	AWeaponBase* GetMainWeapon() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	UWeaponDataAsset* GetCurrentWeaponData() const;

	void ApplyRecoil(const FRecoilData& Recoil);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetActiveThrowable(UWeaponDataAsset* NewThrowableData);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Throwable")
	AWeaponBase* GetThrowableWeapon() const { return ThrowableWeapon; }

	// ─── Mission System ───

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	UMissionDataAsset* CurrentMissionData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Objective")
	int32 CurrentMissionIndex = 0;

	UFUNCTION(BlueprintCallable, Category = "Objective")
	void CompleteCurrentObjective();

	UFUNCTION(BlueprintCallable, Category = "Objective")
	bool IsCurrentObjective(FName GoalID) const;

	UFUNCTION(BlueprintCallable, Category = "Objective")
	FName GetCurrentObjectiveID() const;

	void UpdateMissionObjective();

	UFUNCTION(BlueprintCallable, Category = "Objective")
	void FailMission();

	UFUNCTION(BlueprintCallable, Category = "Objective")
	void FailMissionWithReason(EMissionFailReason Reason);

	UFUNCTION(BlueprintCallable, Category = "Objective")
	float GetRemainingMissionTime() const;

	UFUNCTION(BlueprintCallable, Category = "Objective")
	float GetDistanceToCurrentObjective() const;

	UFUNCTION(BlueprintCallable, Category = "Objective")
	void SetObjective(const FString& NewObjective);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FString CurrentObjective = TEXT("테스트 문구.");

	// ─── Animation & VFX ───

	// Modified: 구르기 상태 플래그 복구
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsRolling = false;

	// Modified: 장착 및 발사 상태 플래그 복구
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	bool bIsEquipped = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	bool bIsFiring = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* DiveRollMontage;
	
	// 수류탄 준비 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* ThrowableMontage;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float UpperBodyBlendWeight = 1.0f;

	UFUNCTION()
	void OnRollMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|UI")
	ECrosshairState CurrentCrosshairState = ECrosshairState::Default;

	UFUNCTION(BlueprintCallable, Category = "Player|UI")
	void NotifyEnemyKilled();

	UFUNCTION(BlueprintCallable, Category = "Player|UI")
	void SetCrosshairState(ECrosshairState NewState);

	UFUNCTION() void ResetCrosshairToDefault();
	void ExecuteThrow();

	// ─── Status Handlers ───

	UFUNCTION(BlueprintCallable)
	void OnDeath();

	UFUNCTION() void HandleHealthChanged(float CurrentHealth, float MaxHealth, const UDamageType* DamageType);
	UFUNCTION() void HandleStaminaChanged(float CurrentStamina, float MaxStamina);
	UFUNCTION() void HandleNoiseChanged(float CurrentNoise, float MaxNoise);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Status")
	bool bIsDead = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	int32 KillCount = 0;

	// Modified: 외부 클래스(MissionRope 등)에서의 접근을 위해 public으로 복구
	float MissionStartTime = 0.0f;
	float MoveSpeed;
	float SprintSpeedMultiplier;
	float SprintSpeed;
	float CameraZOffset = 0.0f;

	// ─── Components (Internal) ───

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	USpringArmComponent* SpringArm;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	USceneComponent* CameraRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UCameraComponent* Camera;

protected:
	// ─── Internal Stats ───
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CrouchBlendSpeed = 5.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float LeanAmount = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MaxLeanOffset = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float LeanSpeed = 3.5f;

	FVector2D RemainingRecoil;
	float RecoilPitchAccum = 0.0f;
	float RecoilRecoverySpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil")
	float RecoilSpeed;

	FTimerHandle KillConfirmTimerHandle;
	FTimerHandle MissionTimerHandle;

private:
	// ─── Weapon Internals ───
	void OnEquipSlotPressed(const struct FInputActionValue& Value);
	void OnEquipNextPressed(const struct FInputActionValue& Value);
	void OnEquipPreviousPressed(const struct FInputActionValue& Value);
	void OnFirePressed(const struct FInputActionValue& Value);
	void OnFireStarted(const struct FInputActionValue& Value);
	void OnFireReleased(const struct FInputActionValue& Value);
	void OnReloadPressed(const struct FInputActionValue& Value);
	void OnThrowableEquipPressed(const struct FInputActionValue& Value);

	void SpawnEquippedWeapons();
	void SpawnThrowableWeapon();
	AWeaponBase* SpawnAndAttachWeapon(UWeaponDataAsset* Data);
	
	UFUNCTION() void HandleThrowableDepleted();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatManager> CombatManager;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UWeaponDataAsset>> EquippedWeapons;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AWeaponBase> WeaponBaseClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<AWeaponBase>> SpawnedWeapons;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AWeaponBase> CurrentWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	int32 CurrentWeaponIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Throwable", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponDataAsset> CurrentThrowableData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Throwable", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponDataAsset> RockData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Throwable", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponDataAsset> GrenadeData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Throwable", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AWeaponBase> ThrowableWeapon;

	int32 PreviousWeaponIndex = INDEX_NONE;

	// ─── Sub-Components ───

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaminaComponent> StaminaComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInteractionComponent> InteractionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNoiseComponent> NoiseComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UVisibilityComponent> VisibilityComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USoundBase> MoveSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USoundBase> RunSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USoundBase> JumpSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USoundBase> RollSound;
};
