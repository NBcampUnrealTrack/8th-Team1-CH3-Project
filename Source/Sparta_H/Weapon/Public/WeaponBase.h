#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "WeaponBase.generated.h"

class USkeletalMeshComponent;
class UAmmoComponent;
class UWeaponDataAsset;

// 투척 무기의 보유 수가 0에 도달했을 때 브로드캐스트. PlayerCharacter에서 슬롯 자동 복귀 처리
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnThrowableDepleted);

// 모든 무기의 베이스 액터. DA를 받아 메시/탄약을 초기화하고,
// 캐릭터의 GripPoint 소켓에 부착되어 Show/Hide로 슬롯 상태를 표현
UCLASS()
class SPARTA_H_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();

	// 차징 중 궤적 갱신을 위해 Tick 활성 (생성자에서 bCanEverTick = true)
	virtual void Tick(float DeltaTime) override;

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

	// 1발 발사. 상태/탄약/쿨다운을 가드하고 1인칭 팔에 발사 몽타주 재생
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Fire();

	// 재장전 시작. 상태/탄창 가드 후 Reloading 전이 + 1인칭 팔 몽타주 재생, ReloadTime 후 자동 완료
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Reload();

private:
	// FireRate 쿨다운 종료 시 Idle 복귀 + bCanFire 해제
	void OnFireCooldownEnded();

	// ReloadTime 종료 시 탄창 채우고 Idle 복귀. Reload 도중 Swap 들어왔으면 Swap 상태 보존
	void OnReloadCompleted();

	// 메시 자체를 루트로 — 캐릭터 GripPoint에 SnapToTarget으로 부착하면 메시가 소켓 트랜스폼에 맞춰짐
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> WeaponMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAmmoComponent> AmmoComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponDataAsset> WeaponData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	EWeaponState CurrentWeaponState = EWeaponState::Idle;

	// FireRate 동안 다음 발사 차단. 풀오토/세미오토 모두 동일 쿨다운 사용
	bool bCanFire = true;

	FTimerHandle FireCooldownTimerHandle;
	FTimerHandle ReloadTimerHandle;
	
private:
	// ThrowCooldown 종료 시 bThrowOnCooldown 해제
	void OnThrowCooldownEnded();

	// 차징 중 매 프레임 호출 — PredictProjectilePath 결과를 DrawDebugLine 으로 그림
	void UpdateTrajectoryPreview();

	// 현재 보유 수. Initialize 시 DA->MaxStockCount 로 세팅 (음수면 무한)
	int32 CurrentStock = 0;

	bool bIsChargingThrow = false;
	bool bThrowOnCooldown = false;

	// 0~1 차징 진행도. ChargeDuration 동안 1.0 까지 증가
	float CurrentChargeAlpha = 0.0f;

	FTimerHandle ThrowCooldownTimerHandle;

public:
	// LMB 누름 — 차징 시작. CanThrow 실패 시 무시
	UFUNCTION(BlueprintCallable, Category = "Weapon|Throwable")
	void BeginThrowCharge();

	// LMB 뗌 — 실제 투척. 현재 차징 알파에 비례한 속도로 발사 후 스택 차감 + 쿨다운
	UFUNCTION(BlueprintCallable, Category = "Weapon|Throwable")
	void ReleaseThrow();

	// 무기 교체 등으로 차징을 강제 취소 (스택/쿨다운은 건드리지 않음)
	UFUNCTION(BlueprintCallable, Category = "Weapon|Throwable")
	void CancelThrowCharge();

	// Stock != 0 (또는 무한) && !bThrowOnCooldown
	UFUNCTION(BlueprintCallable, Category = "Weapon|Throwable")
	bool CanThrow() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon|Throwable")
	int32 GetCurrentStock() const { return CurrentStock; }

	// DA->MaxStockCount == -1 인 경우(돌맹이 등)
	UFUNCTION(BlueprintCallable, Category = "Weapon|Throwable")
	bool HasUnlimitedStock() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon|Throwable")
	bool IsChargingThrow() const { return bIsChargingThrow; }

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Throwable")
	FOnThrowableDepleted OnDepleted;
};