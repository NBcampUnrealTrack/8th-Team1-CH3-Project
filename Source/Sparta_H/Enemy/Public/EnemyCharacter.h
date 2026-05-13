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
    FAlertLevelStats SuspiciousStats = { 100.f, 1500.f,  800.f, 600.f };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|AlertStats")
    FAlertLevelStats CombatStats     = { 100.f, 2000.f, 1000.f, 800.f };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|AlertStats")
    FAlertLevelStats LostStats       = { 100.f, 2000.f, 1000.f, 800.f };

private:
    // ---------------------------------------------------------------
    // 전투 수치
    // ---------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat", meta = (AllowPrivateAccess = "true"))
    float FireRange = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat", meta = (AllowPrivateAccess = "true"))
    float FireAngleLimit = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat", meta = (AllowPrivateAccess = "true"))
    float HitAccuracy = 0.3f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Combat", meta = (AllowPrivateAccess = "true"))
    float WeaponDamage = 15.0f;

    // ---------------------------------------------------------------
    // AlertRange
    // ---------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Alert", meta = (AllowPrivateAccess = "true"))
    float CombatAlertRange = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Alert", meta = (AllowPrivateAccess = "true"))
    float LostAlertRange = 1000.0f;

    void AlertNearbyEnemies(AActor* TargetPlayer, float AlertRange, EAlertLevel NewLevel);
    void ApplyPerceptionStats(const FAlertLevelStats& Stats);

    // ---------------------------------------------------------------
    // 머리 위 아이콘
    // ---------------------------------------------------------------
    void UpdateAlertIcon(EAlertLevel NewLevel);
    void HideAlertIcon();

    // ?? / !! 표시 후 자동 숨김 딜레이 (에디터에서 조정 가능)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|UI", meta = (AllowPrivateAccess = "true"))
    float IconHideDelay = 3.0f;

    FTimerHandle IconHideTimerHandle;

    // ---------------------------------------------------------------
    // 자동 복귀 타이머
    // ---------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Alert", meta = (AllowPrivateAccess = "true"))
    float SuspiciousRevertDelay = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Alert", meta = (AllowPrivateAccess = "true"))
    float LostRevertDelay = 15.0f;

    FTimerHandle SuspiciousRevertTimerHandle;
    FTimerHandle LostRevertTimerHandle;

    void OnSuspiciousRevertTimerExpired();
    void OnLostRevertTimerExpired();

    // ---------------------------------------------------------------
    // 탐지 확정 타이머
    // ---------------------------------------------------------------
    FTimerHandle DetectionTimerHandle;

    UPROPERTY()
    AActor* SuspectedTarget = nullptr;

    void OnDetectionTimerExpired();
};