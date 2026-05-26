#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "WeaponBase.generated.h"

class UAnimMontage;
class UNiagaraSystem;
class UNiagaraComponent;
class USoundBase;

class USceneComponent;
class USkeletalMeshComponent;
class UAmmoComponent;
class UWeaponDataAsset;

// Modified: 소유자 캐릭터 참조를 위한 전방 선언 추가
class APlayerCharacter;

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

	// 차징 중 궤적 갱신을 위해 Tick 활성 (생성자에서 bCanEverTick = true, 평소엔 끈 상태)
	virtual void Tick(float DeltaTime) override;

	// ─── 공통 ───

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

	// DA를 받아 WeaponData 포인터 세팅 + 탄약/스택 초기화. 메시/몽타주는 BP에서 설정하므로 건드리지 않음
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Initialize(UWeaponDataAsset* InWeaponData);

	// ─── 발사 / 재장전 ───

	// 1발 발사. 상태/탄약/쿨다운을 가드하고 1인칭 팔에 발사 몽타주 재생
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Fire();

	// 재장전 시작. 상태/탄창 가드 후 Reloading 전이 + 1인칭 팔 몽타주 재생, ReloadTime 후 자동 완료
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Reload();

	// ─── 투척 ───

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

	UFUNCTION(BlueprintCallable, Category = "Weapon|Throwable")
	float GetThrowCooldownRemaining() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon|Throwable")
	float GetThrowCooldownPercent() const;

	// Modified: 리스폰 시 상태 초기화를 위한 함수 추가
	UFUNCTION(BlueprintCallable, Category = "Weapon|Throwable")
	void ResetThrowCooldown();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Throwable")
	void RefillStock();

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Throwable")
	FOnThrowableDepleted OnDepleted;

	// ─── 애니메이션 / 사운드 / VFX (BP에서 세팅) ───

	// 1인칭 팔(Mesh1P)에 재생할 발사 몽타주. 풀바디 임포트 애니는 팔 스켈레톤으로 리타게팅 후 몽타주화
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
	TObjectPtr<UAnimMontage> FireMontage1P;
	
	// 1인칭 팔(Mesh1P)에 재생할 재장전 몽타주. 길이는 DA의 ReloadTime과 맞춰 세팅
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage1P;

	// 발사 시 재생할 사운드. 플레이어가 듣는 청각 피드백 — AI 어그로 노이즈(EmitNoise)와는 별개
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Audio")
	TObjectPtr<USoundBase> FireSound;

	// 발사 시 무기 메시의 MuzzleSocketName 위치에 스폰할 총구 화염 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|VFX")
	TObjectPtr<UNiagaraSystem> MuzzleFlashEffect;

	// 라인 트레이스 적중점에 스폰할 임팩트 이펙트 (피격 표면 스파크/데칼 등)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|VFX")
	TObjectPtr<UNiagaraSystem> ImpactVFX;

	// 무기 스켈레탈 메시에 만들어 둔 총구 소켓 이름. 메시별로 다를 수 있어 DA에서 지정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|VFX")
	FName MuzzleSocketName = TEXT("Muzzle");

protected:
	// Modified: BeginPlay를 오버라이드하여 오너 캐싱 및 에셋 프리로드 수행
	virtual void BeginPlay() override;

private:
	// Modified: 매번 Cast하는 비용을 줄이기 위해 소유자 캐릭터를 캐싱
	UPROPERTY()
	TObjectPtr<APlayerCharacter> OwnerCharacter;
	
	// 카메라(에이밍) 시점의 시작 위치와 방향을 가져옴. 소유자/PC/CameraManager 중 하나라도 없으면 false
	bool GetAimStartAndDirection(FVector& OutStart, FVector& OutDirection) const;

	// FireRate 쿨다운 종료 시 Idle 복귀 + bCanFire 해제
	void OnFireCooldownEnded();

	// ReloadTime 종료 시 탄창 채우고 Idle 복귀. Reload 도중 Swap 들어왔으면 Swap 상태 보존
	void OnReloadCompleted();

	// ThrowCooldown 종료 시 bThrowOnCooldown 해제
	void OnThrowCooldownEnded();

	// 차징 중 매 프레임 호출 — PredictProjectilePath 결과를 DrawDebugLine 으로 그림
	void UpdateTrajectoryPreview();

	// ─── 컴포넌트 ───

	// 빈 루트 — GripPoint 소켓에 스냅되는 기준점. 메시는 자식으로 달려 BP에서 오프셋 조정 가능
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> WeaponRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> WeaponMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAmmoComponent> AmmoComponent;

	// ─── 런타임 상태 (발사/재장전) ───

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponDataAsset> WeaponData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	EWeaponState CurrentWeaponState = EWeaponState::Idle;

	// FireRate 동안 다음 발사 차단. 풀오토/세미오토 모두 동일 쿨다운 사용
	bool bCanFire = true;

	FTimerHandle FireCooldownTimerHandle;
	FTimerHandle ReloadTimerHandle;

	// ─── 런타임 상태 (투척) ───

	// 현재 보유 수. Initialize 시 DA->MaxStockCount 로 세팅 (음수면 무한)
	int32 CurrentStock = 0;

	bool bIsChargingThrow = false;
	bool bThrowOnCooldown = false;

	// 0~1 차징 진행도. ChargeDuration 동안 1.0 까지 증가
	float CurrentChargeAlpha = 0.0f;

	FTimerHandle ThrowCooldownTimerHandle;
	
	TArray<TObjectPtr<UNiagaraComponent>> TrajectoriesComponents;
	void ClearTrajectoryComponents();
};
