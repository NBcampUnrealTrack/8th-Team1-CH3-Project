#pragma once

#include "CoreMinimal.h"
#include "EnemyCharacter.h"
#include "Components/WidgetComponent.h"
#include "BossEnemy.generated.h"

class AThrowableActor;

UENUM(BlueprintType)
enum class EBossPhase : uint8
{
	Phase1 UMETA(DisplayName = "Phase 1 - Burst"),
	Phase2 UMETA(DisplayName = "Phase 2 - Grenade + Reinforce"),
	Phase3 UMETA(DisplayName = "Phase 3 - All Patterns")
};

UCLASS()
class SPARTA_H_API ABossEnemy : public AEnemyCharacter
{
	GENERATED_BODY()

public:
	ABossEnemy();

	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	                         AController* EventInstigator, AActor* DamageCauser) override;

	virtual void OnAlertLevelChanged(EAlertLevel NewLevel) override;

	UFUNCTION(BlueprintCallable, Category = "Boss")
	EBossPhase GetCurrentPhase() const { return CurrentPhase; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void InitializeStats() override;
	virtual void Die() override;

	// ─── Phase ───────────────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Phase")
	EBossPhase CurrentPhase = EBossPhase::Phase1;

	void TryPhaseTransition();
	void EnterPhase2();
	void EnterPhase3();

	// ─── Burst Fire (5발 연사) ────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack")
	float BurstInterval = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack")
	float BurstShotInterval = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack")
	int32 MaxBurstShots = 5;

	int32 BurstShotCount = 0;
	FTimerHandle BurstCycleHandle;
	FTimerHandle BurstShotHandle;
	AActor* BossTarget = nullptr;

	// 특수 공격(정밀사격, 수류탄) 중 버스트 차단용
	bool bIsPerformingSpecialAttack = false;

	void StartBurstCycle();
	void ExecuteBurstStep();

	// ─── Grenade ─────────────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack")
	TSubclassOf<AThrowableActor> GrenadeClass;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack")
	float GrenadeInterval = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack")
	float GrenadeThrowSpeed = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Animation")
	TObjectPtr<UAnimMontage> GrenadeMontage;

	FTimerHandle GrenadeHandle;
	void ThrowGrenade();
	void SpawnGrenade();

	// ─── Precision Shot (선딜 고데미지) ───────────────────────────────────
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack")
	float PrecisionDamage = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack")
	float PrecisionAimDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack")
	float PrecisionInterval = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Animation")
	TObjectPtr<UAnimMontage> PrecisionMontage;

	FTimerHandle PrecisionCycleHandle;
	FTimerHandle PrecisionFireHandle;
	float PrecisionAimElapsed = 0.f;
	bool bIsAiming = false;

	void StartPrecisionCycle();
	void StartPrecisionAim();
	void ExecutePrecisionFire();

	// ─── Montage End ──────────────────────────────────────────────────────
	UFUNCTION()
	void OnSpecialMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// ─── Reinforcement ────────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Reinforce")
	TSubclassOf<AEnemyCharacter> ReinforcementClass;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Reinforce")
	int32 ReinforcementCount = 2;

	bool bHasSpawnedReinforcement = false;
	void SpawnReinforcement();

	// ─── Precision Warning Widget (머리 위 경고) ──────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|UI")
	UWidgetComponent* PrecisionWarningWidgetComp;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|UI")
	float PrecisionWidgetHeightOffset = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|UI")
	FVector2D PrecisionWidgetDrawSize = FVector2D(200.f, 60.f);

	UFUNCTION(BlueprintPure, Category = "Boss|UI")
	class UBossPrecisionWidget* GetPrecisionWidget() const;

	// ─── Reward ───────────────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Reward")
	TSubclassOf<AActor> RewardClass;

	void SpawnReward();

	// ─── Perception Override ──────────────────────────────────────────────
	UFUNCTION()
	void OnBossTargetPerceived(AActor* Actor, FAIStimulus Stimulus);

	// BT의 StartFirePattern 호출 차단 — 보스는 C++ BurstCycle이 직접 처리
	virtual void StartFirePattern(AActor* TargetActor) override;

	// ─── Debug ────────────────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase")
	float Phase2HPRatio = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase")
	float Phase3HPRatio = 0.3f;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Boss|Debug")
	void Debug_ForcePhase2();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Boss|Debug")
	void Debug_ForcePhase3();
};
