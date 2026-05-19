#pragma once

#include "CoreMinimal.h"
#include "EnemyCharacter.h"
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

    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    virtual void OnAlertLevelChanged(EAlertLevel NewLevel) override;

    UFUNCTION(BlueprintCallable, Category = "Boss")
    EBossPhase GetCurrentPhase() const { return CurrentPhase; }
    
    UFUNCTION(BlueprintImplementableEvent, Category = "Boss")
    void ShowBossAlert(const FString& Message, float Duration);

protected:
    virtual void BeginPlay() override;
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

    void StartBurstCycle();
    void ExecuteBurstStep();

    // ─── Grenade ─────────────────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack")
    TSubclassOf<AThrowableActor> GrenadeClass;

    UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack")
    float GrenadeInterval = 10.f;

    UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack")
    float GrenadeThrowSpeed = 1200.f;

    FTimerHandle GrenadeHandle;
    void ThrowGrenade();

    // ─── Precision Shot (선딜 고데미지) ───────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack")
    float PrecisionDamage = 80.f;

    UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack")
    float PrecisionAimDuration = 1.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack")
    float PrecisionInterval = 15.f;

    FTimerHandle PrecisionCycleHandle;
    FTimerHandle PrecisionFireHandle;

    void StartPrecisionCycle();
    void StartPrecisionAim();
    void ExecutePrecisionFire();

    // ─── Reinforcement ────────────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category = "Boss|Reinforce")
    TSubclassOf<AEnemyCharacter> ReinforcementClass;

    UPROPERTY(EditDefaultsOnly, Category = "Boss|Reinforce")
    int32 ReinforcementCount = 2;

    bool bHasSpawnedReinforcement = false;
    void SpawnReinforcement();

    // ─── Reward ───────────────────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category = "Boss|Reward")
    TSubclassOf<AActor> RewardClass;

    void SpawnReward();

    // ─── Perception Override ──────────────────────────────────────────────
    UFUNCTION()
    void OnBossTargetPerceived(AActor* Actor, FAIStimulus Stimulus);

private:

    static constexpr float Phase2HPRatio = 0.6f;
    static constexpr float Phase3HPRatio = 0.3f;
};