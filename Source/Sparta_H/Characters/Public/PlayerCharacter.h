// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WeaponTypes.h"
#include "Systems/Public/CombatTypes.h"
#include "PlayerCharacter.generated.h"

struct FInputActionValue;
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

// 목표 변경 시 호출될 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObjectiveChanged, const FString&, NewObjective);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMissionStateChanged);

// 크로스헤어 상태 변경 시 호출될 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCrosshairStateChangedDelegate, ECrosshairState, NewState);

// 플레이어 대미지 발생 시 비네트 등 효과 처리를 위한 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerTakeDamage, float, DamageAmount);


// 체크포인트 저장을 위한 구조체
USTRUCT(BlueprintType)
struct FPlayerCheckpointData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MissionIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasRifle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator Rotation = FRotator::ZeroRotator;
};

UCLASS()
class SPARTA_H_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerCharacter();

	// Modified: 라이플 보유 여부 플래그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	bool bHasRifle = false;

	/** 체크포인트 저장/로드 **/
	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	FPlayerCheckpointData SaveCheckpoint();

	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	void LoadCheckpoint(const FPlayerCheckpointData& CheckpointData);

	//스프링 암
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	USpringArmComponent* SpringArm;

	// 1인칭 카메라 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UCameraComponent* Camera;

	float MoveSpeed;
	float SprintSpeedMultiplier;
	float SprintSpeed;

	// 총기 착용 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	bool bIsEquipped = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	bool bHasRifle;

	// 구르기 관련 , 일회성 동작이므로 몽타주 사용
	bool bIsRolling = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* DiveRollMontage;
	UFUNCTION()
	void OnRollMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	//기울이기 관련
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float LeanAmount = 0.f;
	/** 기울임 시 카메라가 옆으로 이동할 최대 거리 (예: 100.0) **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float MaxLeanOffset = 25.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float LeanSpeed = 3.5f;

	float CameraZOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CrouchBlendSpeed = 5.0f; // 카메라가 부드럽게 움직이는 속

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void Jump() override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// 다른 클래스에서 캐릭터의 컴포넌트를 참조하기 위한 게터함수
	UHealthComponent* GetHealthComponent() const { return HealthComponent; }
	UStaminaComponent* GetStaminaComponent() const { return StaminaComponent; }
	UInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }
	UNoiseComponent* GetNoiseComponent() const { return NoiseComponent; }
	UVisibilityComponent* GetVisibilityComponent() const { return VisibilityComponent; }

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
	void StartLeanRight(const FInputActionValue& value);
	UFUNCTION()
	void StartLeanLeft(const FInputActionValue& value);
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

	/** 현재 슬롯에 선택된 메인 무기(총기/칼)를 반환. 투척 무기를 들고 있어도 슬롯 무기를 반환함 **/
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	AWeaponBase* GetMainWeapon() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	UWeaponDataAsset* GetCurrentWeaponData() const;

	void ApplyRecoil(const FRecoilData& Recoil);

	// 미션 진입 시 호출 — 현재 투척물을 새 DA로 교체. 들고 있던 경우 이전 슬롯 자동 복귀
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetActiveThrowable(UWeaponDataAsset* NewThrowableData);
	/** End of Weapon System **/

	// --- 미션 시스템 추가 ---
	// 데이터 에셋 기반 미션 정보
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	class UMissionDataAsset* CurrentMissionData;

	// 현재 진행 중인 미션 인덱스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Objective")
	int32 CurrentMissionIndex = 0;

	// 현재 목표를 완료하고 다음 목표로 진행
	UFUNCTION(BlueprintCallable, Category = "Objective")
	void CompleteCurrentObjective();

	// 특정 ID의 목표가 현재 진행 중인지 확인
	UFUNCTION(BlueprintCallable, Category = "Objective")
	bool IsCurrentObjective(FName GoalID) const;

	// 현재 진행 중인 목표의 ID 반환
	UFUNCTION(BlueprintCallable, Category = "Objective")
	FName GetCurrentObjectiveID() const;

	// 미션 데이터로부터 현재 목표 텍스트 업데이트
	void UpdateMissionObjective();
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator,
	                         AActor* DamageCauser) override;

	// 미션 성공/실패 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Objective")
	FOnMissionStateChanged OnMissionCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Objective")
	FOnMissionStateChanged OnMissionFailed;

	// 미션 실패 처리
	UFUNCTION(BlueprintCallable, Category = "Objective")
	void FailMission();

	/** 원인과 함께 미션 실패 처리 **/
	UFUNCTION(BlueprintCallable, Category = "Objective")
	void FailMissionWithReason(EMissionFailReason Reason);

	// 현재 남은 미션 시간 반환
	UFUNCTION(BlueprintCallable, Category = "Objective")
	float GetRemainingMissionTime() const;

	// 현재 목표 지점까지의 거리 반환 (미터 단위)
	UFUNCTION(BlueprintCallable, Category = "Objective")
	float GetDistanceToCurrentObjective() const;

	// 목표를 유동적으로 변경하기 위한 함수 (기존 유지)
	UFUNCTION(BlueprintCallable, Category = "Objective")
	void SetObjective(const FString& NewObjective);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FString CurrentObjective = TEXT("테스트 문구.");

	UPROPERTY(BlueprintAssignable, Category = "Objective")
	FOnObjectiveChanged OnObjectiveChanged;

	UPROPERTY(BlueprintAssignable, Category = "Player|UI")
	FOnCrosshairStateChangedDelegate OnCrosshairStateChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|UI")
	ECrosshairState CurrentCrosshairState = ECrosshairState::Default;

	UFUNCTION(BlueprintCallable, Category = "Player|UI")
	void NotifyEnemyKilled();

	/** 크로스헤어 상태를 변경하고 알림을 보냄 **/
	UFUNCTION(BlueprintCallable, Category = "Player|UI")
	void SetCrosshairState(ECrosshairState NewState);

	/** 처치 확인 크로스헤어 상태를 원복하기 위한 함수 **/
	UFUNCTION()
	void ResetCrosshairToDefault();

	/** 처치 확인용 타이머 핸들 **/
	FTimerHandle KillConfirmTimerHandle;

	// 대미지 수신 시 호출될 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Player|Effects")
	FOnPlayerTakeDamage OnPlayerTakeDamage;

	/** End of 플레이어 스탯 / 목표 **/

	/** 사망 시 호출될 함수 **/
	UFUNCTION(BlueprintCallable)
	void OnDeath();

	/** 사망 상태를 확인하는 플래그 (중복 사망 처리 방지) **/
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Status")
	bool bIsDead = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<class USoundBase> MoveSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<class USoundBase> RunSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<class USoundBase> JumpSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<class USoundBase> RollSound;

	/** 적 처치 횟수 **/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	int32 KillCount = 0;

	/** 미션 시작 시간 **/
	float MissionStartTime = 0.0f;

protected:
	FVector2D TargetRecoil;
	FVector2D CurrentRecoil;
	FVector2D RemainingRecoil;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil")
	float RecoilSpeed;

private:
	/** Weapon Input Callbacks **/
	void OnEquipSlotPressed(const FInputActionValue& Value);
	void OnEquipNextPressed(const FInputActionValue& Value);
	void OnEquipPreviousPressed(const FInputActionValue& Value);
	void OnFirePressed(const FInputActionValue& Value);
	void OnFireStarted(const FInputActionValue& Value);
	void OnFireReleased(const FInputActionValue& Value);
	void OnReloadPressed(const FInputActionValue& Value);
	void OnThrowableEquipPressed(const FInputActionValue& Value);

	// BeginPlay에서 EquippedWeapons 각 DA로 무기 액터를 스폰해 본체 메시 GripPoint에 부착
	void SpawnEquippedWeapons();

	// CurrentThrowableData 로 ThrowableWeapon 액터를 스폰. 이미 있으면 무시
	void SpawnThrowableWeapon();

	// ThrowableWeapon 보유 수 0 도달 시 호출 — PreviousWeaponIndex 슬롯으로 복귀
	UFUNCTION()
	void HandleThrowableDepleted();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatManager> CombatManager;

	// 슬롯에 등록할 무기 DA. BP 디테일에서 1/2/3 슬롯(Pistol/Rifle/Knife)에 등록
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

	// G키로 들 투척 무기 기본 DA. 미션 진행에 따라 SetActiveThrowable 로 변경
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Throwable", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponDataAsset> CurrentThrowableData;

	/** 미션별 투척 무기 데이터 **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Throwable", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponDataAsset> RockData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Throwable", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponDataAsset> GrenadeData;

	// SpawnEquippedWeapons 직후 스폰된 투척 무기 액터. SpawnedWeapons 와는 별도 슬롯
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Throwable", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AWeaponBase> ThrowableWeapon;

public:
	UFUNCTION(BlueprintCallable, Category = "Weapon|Throwable")
	AWeaponBase* GetThrowableWeapon() const { return ThrowableWeapon; }

	// G 누르기 직전 들고 있던 슬롯 인덱스. 0개 소진 시 자동 복귀용
	int32 PreviousWeaponIndex = 0;

	// 회복이 남아있는 누적 pitch (양수 = 위로 튕긴 양)
	float RecoilPitchAccum = 0.0f;
	// 현재 장창 무기의 회복 속도
	float RecoilRecoverySpeed = 0.0f;

	// 미션 타이머 핸들
	FTimerHandle MissionTimerHandle;

	/** Character Components 추가 **/
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
};
