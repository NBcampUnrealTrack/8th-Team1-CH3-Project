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
class USkeletalMeshComponent;

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

    bool CanShootTarget(AActor* TargetActor, AActor** OutBlocker = nullptr);

    // 3점사 패턴 시작 함수 (Behavior Tree 등에서 호출)
    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    virtual void StartFirePattern(AActor* TargetActor);

    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    bool FireAtTarget(AActor* TargetActor);
    
    UFUNCTION(BlueprintCallable, Category = "AI|Patrol")
    FVector GetPatrolWorldLocationA() const { return PatrolSpawnLocation + PatrolOffsetA; }

    UFUNCTION(BlueprintCallable, Category = "AI|Patrol")
    FVector GetPatrolWorldLocationB() const { return PatrolSpawnLocation + PatrolOffsetB; }

    UFUNCTION(BlueprintCallable, Category = "AI")
    UBehaviorTree* GetEnemyBT() const { return EnemyBT; }

    // 소환 등 특수 상황에서 머리 위에 !! 아이콘을 Duration초 동안 표시
    UFUNCTION(BlueprintCallable, Category = "AI|UI")
    void ShowExclamationIcon(float Duration);
    
    virtual void OnConstruction(const FTransform& Transform) override;

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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Weapon")
    USkeletalMeshComponent* WeaponMeshComp;

    // ---------------------------------------------------------------
    // 머리 위 아이콘 위젯 (WBP_EnemyAlertIcon 할당)
    // ---------------------------------------------------------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|UI")
    UWidgetComponent* AlertIconWidgetComp;

    UPROPERTY(EditDefaultsOnly, Category = "AI|UI")
    float AlertIconHeightOffset = 120.f;

    UFUNCTION()
    void OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus);

    // ---------------------------------------------------------------
    // AlertLevel별 Perception 수치 (기획서 기본값)
    // ---------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|AlertStats")
    FAlertLevelStats IdleStats       = { 60.f,  600.f,  3000.f, 400.f };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|AlertStats")
    FAlertLevelStats SuspiciousStats = { 180.f, 1500.f, 5000.f, 600.f };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|AlertStats")
    FAlertLevelStats CombatStats     = { 180.f, 2000.f, 8000.f, 800.f };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|AlertStats")
    FAlertLevelStats LostStats       = { 100.f, 2000.f, 8000.f, 800.f };

    UPROPERTY(EditAnywhere, Category = "AI")
    class UBehaviorTree* EnemyBT;

    UPROPERTY(EditAnywhere, Category = "AI|VFX")
    class UNiagaraSystem* MuzzleFlashEffect;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Patrol")
    bool bUseRandomPatrol = true;

private:
    // ---------------------------------------------------------------
    // 전투 수치
    // ---------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat", meta = (AllowPrivateAccess = "true"))
    float FireRange = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat", meta = (AllowPrivateAccess = "true"))
    float FireAngleLimit = 90.0f;

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

    // --- 시야 확보 재배치 ---
    bool bIsRepositioning = false;
    FTimerHandle RepositionTimerHandle;
    void TryRepositionForShot();          // 시야 막힘 시 이동 후 재시도
    
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

    FTimerHandle SuspiciousRevertTimerHandle;

    void OnSuspiciousRevertTimerExpired();

    // ---------------------------------------------------------------
    // 탐지 확정 타이머
    // ---------------------------------------------------------------
    FTimerHandle DetectionTimerHandle;

    // --- 타겟 관리 ---
    UPROPERTY()
    AActor* SuspectedTarget = nullptr;

    void OnDetectionTimerExpired();
    
    // 패트롤 오프셋 (스폰 위치 기준 상대 좌표, 에디터에서 조정 가능)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Patrol", meta = (AllowPrivateAccess = "true"))
    FVector PatrolOffsetA = FVector(300.f, 0.f, 0.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Patrol", meta = (AllowPrivateAccess = "true"))
    FVector PatrolOffsetB = FVector(-300.f, 0.f, 0.f);

    FVector PatrolSpawnLocation;
    
    // 20초 동안 플레이어를 놓쳤을 때 호출될 함수
    void OnCombatToLostTimerExpired();
    
    FTimerHandle CombatToLostTimerHandle;
    
};