#pragma once

#include "CoreMinimal.h"
#include "BaseEnemy.h"
#include "CombatManager.h"
#include "EnemyAlertWidget.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyCharacter.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UWidgetComponent;
class UAnimMontage;

// ---------------------------------------------------------------
// AlertLevel별 Perception + 이동 수치 묶음
// ---------------------------------------------------------------
USTRUCT(BlueprintType)
struct FAlertLevelStats
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FOVAngle = 100.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SightRange = 1000.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HearingRange = 500.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MoveSpeed = 400.0f;
};

UCLASS()
class SPARTA_H_API AEnemyCharacter : public ABaseEnemy
{
    GENERATED_BODY()

public:
    AEnemyCharacter();

    virtual void OnAlertLevelChanged(EAlertLevel NewLevel) override;

    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    bool CanShootTarget(AActor* TargetActor);

    // 3점사 패턴 시작 함수 (Behavior Tree 등에서 호출)
    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    void StartFirePattern(AActor* TargetActor);

    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    bool FireAtTarget(AActor* TargetActor);

protected:
    virtual void BeginPlay() override;
    virtual void InitializeStats() override;
    virtual void Die() override;

    UPROPERTY(EditDefaultsOnly, Category = "AI|Animation")
    UAnimMontage* DeathMontage;

    UPROPERTY(EditDefaultsOnly, Category = "AI|Animation")
    UAnimMontage* FireMontage;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAIPerceptionComponent* AIPerceptionComp;

    UPROPERTY()
    UAISenseConfig_Sight* SightConfig;

    UPROPERTY()
    UAISenseConfig_Hearing* HearingConfig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Combat")
    UCombatManager* CombatManagerComp;

    // ---------------------------------------------------------------
    // 머리 위 아이콘 위젯 (WBP_EnemyAlertIcon 할당)
    // ---------------------------------------------------------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|UI")
    UWidgetComponent* AlertIconWidgetComp;

    UFUNCTION()
    void OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus);

    // ---------------------------------------------------------------
    // AlertLevel별 Perception 수치 (기획서 기본값)
    // ---------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|AlertStats")
    FAlertLevelStats IdleStats       = { 100.f, 1000.f,  500.f, 400.f };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|AlertStats")
    FAlertLevelStats SuspiciousStats = { 180.f, 1500.f,  800.f, 600.f };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|AlertStats")
    FAlertLevelStats CombatStats     = { 180.f, 2000.f, 1000.f, 800.f };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|AlertStats")
    FAlertLevelStats LostStats       = { 100.f, 2000.f, 1000.f, 800.f };

    UPROPERTY(EditAnywhere, Category = "AI")
    class UBehaviorTree* EnemyBT;
private:
    // --- 전투 수치 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat", meta = (AllowPrivateAccess = "true"))
    float FireRange = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat", meta = (AllowPrivateAccess = "true"))
    float FireAngleLimit = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat", meta = (AllowPrivateAccess = "true"))
    float HitAccuracy = 0.3f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Combat", meta = (AllowPrivateAccess = "true"))
    float WeaponDamage = 15.0f;

    // --- 기획 추가: 확률 기반 감지 (Level 2 -> 3) ---
    float SpotProb = 0.7f;                // 초기값 0.7
    FTimerHandle SpotCheckTimerHandle;    // 1초마다 반복될 핸들
    void ProcessSpotCheck();              // 확률 계산 로직

    // --- 기획 추가: 사격 패턴 (3회 사격 로직) ---
    int32 CurrentShotCount = 0;           // 현재 발사 횟수
    FTimerHandle FirePatternTimerHandle;  // 0.4초/0.8초 제어용
    void ExecuteFireStep();               // 실제 한 발씩 쏘는 단계

    // --- AlertRange & 동기화 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Alert", meta = (AllowPrivateAccess = "true"))
    float CombatAlertRange = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Alert", meta = (AllowPrivateAccess = "true"))
    float LostAlertRange = 1000.0f;

    void AlertNearbyEnemies(AActor* TargetPlayer, float AlertRange, EAlertLevel NewLevel);
    void ApplyPerceptionStats(const FAlertLevelStats& Stats);

    // --- 머리 위 아이콘 ---
    void UpdateAlertIcon(EAlertLevel NewLevel);
    void HideAlertIcon();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|UI", meta = (AllowPrivateAccess = "true"))
    float IconHideDelay = 3.0f;

    FTimerHandle IconHideTimerHandle;

    // --- 자동 복귀 타이머 (기획: Lost는 20초) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Alert", meta = (AllowPrivateAccess = "true"))
    float SuspiciousRevertDelay = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Alert", meta = (AllowPrivateAccess = "true"))
    float LostRevertDelay = 20.0f; // 기획서에 맞춰 20초로 변경

    FTimerHandle SuspiciousRevertTimerHandle;
    FTimerHandle LostRevertTimerHandle;

    void OnSuspiciousRevertTimerExpired();
    void OnLostRevertTimerExpired();

    // --- 타겟 관리 ---
    UPROPERTY()
    AActor* SuspectedTarget = nullptr;
};