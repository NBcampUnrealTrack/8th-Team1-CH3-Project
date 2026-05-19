#include "BossEnemy.h"
#include "ThrowableActor.h"
#include "CombatManager.h"
#include "BlackboardKeys.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ABossEnemy::ABossEnemy()
{
}

void ABossEnemy::BeginPlay()
{
    Super::BeginPlay();

    // 부모의 Perception 핸들러를 교체 — Suspicious 단계를 건너뛰고 발각 즉시 Combat 진입
    AIPerceptionComp->OnTargetPerceptionUpdated.RemoveAll(this);
    AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &ABossEnemy::OnBossTargetPerceived);
}

void ABossEnemy::InitializeStats()
{
    Super::InitializeStats(); // DA_Enemy_Boss → MaxHealth=500, WeaponDamage=35

    SightConfig->PeripheralVisionAngleDegrees = 180.f; // FOV 360도 (Half-angle)
    AIPerceptionComp->ConfigureSense(*SightConfig);
    AIPerceptionComp->RequestStimuliListenerUpdate();
}

// ─── Perception ──────────────────────────────────────────────────────────────
void ABossEnemy::OnBossTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
    if (bIsDead || !Actor || !Actor->ActorHasTag(TEXT("Player"))) return;

    AAIController* AIC = Cast<AAIController>(GetController());
    if (!AIC) return;
    UBlackboardComponent* BB = AIC->GetBlackboardComponent();
    if (!BB) return;

    if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            BossTarget = Actor;
            BB->SetValueAsObject(BBKeys::TARGET_ACTOR, Actor);
            BB->SetValueAsVector(BBKeys::LAST_KNOWN_LOCATION, Actor->GetActorLocation());

            if (CurrentAlertLevel != EAlertLevel::Combat)
            {
                OnAlertLevelChanged(EAlertLevel::Combat);
            }
        }
        else if (CurrentAlertLevel == EAlertLevel::Combat)
        {
            OnAlertLevelChanged(EAlertLevel::Lost);
        }
    }
    else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            BB->SetValueAsVector(BBKeys::LAST_KNOWN_LOCATION, Stimulus.StimulusLocation);

            if (CurrentAlertLevel != EAlertLevel::Combat)
            {
                BossTarget = Actor;
                BB->SetValueAsObject(BBKeys::TARGET_ACTOR, Actor);
                OnAlertLevelChanged(EAlertLevel::Combat);
            }
        }
    }
}

// ─── AlertLevel ──────────────────────────────────────────────────────────────
void ABossEnemy::OnAlertLevelChanged(EAlertLevel NewLevel)
{
    Super::OnAlertLevelChanged(NewLevel);

    if (NewLevel == EAlertLevel::Combat)
    {
        StartBurstCycle();

        // 이미 Phase2 이상일 때 전투 재개 시 패턴 복구
        if (CurrentPhase >= EBossPhase::Phase2 && !GetWorldTimerManager().IsTimerActive(GrenadeHandle))
        {
            GetWorldTimerManager().SetTimer(GrenadeHandle, this, &ABossEnemy::ThrowGrenade, GrenadeInterval, true, 3.f);
        }
        if (CurrentPhase == EBossPhase::Phase3 && !GetWorldTimerManager().IsTimerActive(PrecisionCycleHandle))
        {
            StartPrecisionCycle();
        }
    }
    else
    {
        GetWorldTimerManager().ClearTimer(BurstCycleHandle);
        GetWorldTimerManager().ClearTimer(BurstShotHandle);
        GetWorldTimerManager().ClearTimer(GrenadeHandle);
        GetWorldTimerManager().ClearTimer(PrecisionCycleHandle);
        GetWorldTimerManager().ClearTimer(PrecisionFireHandle);
    }
}

// ─── TakeDamage / Die ────────────────────────────────────────────────────────
float ABossEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    float Actual = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    if (!bIsDead)
    {
        TryPhaseTransition();
    }

    return Actual;
}

void ABossEnemy::Die()
{
    GetWorldTimerManager().ClearTimer(BurstCycleHandle);
    GetWorldTimerManager().ClearTimer(BurstShotHandle);
    GetWorldTimerManager().ClearTimer(GrenadeHandle);
    GetWorldTimerManager().ClearTimer(PrecisionCycleHandle);
    GetWorldTimerManager().ClearTimer(PrecisionFireHandle);

    SpawnReward();
    Super::Die();
}

// ─── Phase ───────────────────────────────────────────────────────────────────
void ABossEnemy::TryPhaseTransition()
{
    if (MaxHealth <= 0.f) return;
    const float HPRatio = CurrentHealth / MaxHealth;

    if (CurrentPhase == EBossPhase::Phase1 && HPRatio <= Phase2HPRatio)
    {
        EnterPhase2();
    }
    else if (CurrentPhase == EBossPhase::Phase2 && HPRatio <= Phase3HPRatio)
    {
        EnterPhase3();
    }
}

void ABossEnemy::EnterPhase2()
{
    CurrentPhase = EBossPhase::Phase2;
    ShowBossAlert(TEXT("PHASE 2"), 3.f);

    SpawnReinforcement();

    if (CurrentAlertLevel == EAlertLevel::Combat)
    {
        GetWorldTimerManager().SetTimer(GrenadeHandle, this, &ABossEnemy::ThrowGrenade, GrenadeInterval, true, 3.f);
    }
}

void ABossEnemy::EnterPhase3()
{
    CurrentPhase = EBossPhase::Phase3;
    ShowBossAlert(TEXT("PHASE 3"), 3.f);

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed += 200.f;
    }

    if (CurrentAlertLevel == EAlertLevel::Combat)
    {
        StartPrecisionCycle();
    }
}

// ─── Burst Fire ──────────────────────────────────────────────────────────────
void ABossEnemy::StartBurstCycle()
{
    BurstShotCount = 0;
    ExecuteBurstStep();
}

void ABossEnemy::ExecuteBurstStep()
{
    if (bIsDead || CurrentAlertLevel != EAlertLevel::Combat || !IsValid(BossTarget))
    {
        BurstShotCount = 0;
        return;
    }

    if (BurstShotCount < MaxBurstShots)
    {
        if (CanShootTarget(BossTarget))
        {
            FireAtTarget(BossTarget);
        }
        BurstShotCount++;
        GetWorldTimerManager().SetTimer(BurstShotHandle, this, &ABossEnemy::ExecuteBurstStep, BurstShotInterval, false);
    }
    else
    {
        BurstShotCount = 0;
        GetWorldTimerManager().SetTimer(BurstCycleHandle, this, &ABossEnemy::StartBurstCycle, BurstInterval, false);
    }
}

// ─── Grenade ─────────────────────────────────────────────────────────────────
void ABossEnemy::ThrowGrenade()
{
    if (bIsDead || !GrenadeClass || !IsValid(BossTarget)) return;

    const FVector Origin = GetActorLocation() + FVector(0.f, 0.f, 50.f);
    const FVector ThrowDir = (BossTarget->GetActorLocation() - Origin).GetSafeNormal();

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AThrowableActor* Grenade = GetWorld()->SpawnActor<AThrowableActor>(
        GrenadeClass, Origin, ThrowDir.Rotation(), SpawnParams);

    if (Grenade)
    {
        Grenade->Launch(ThrowDir, GrenadeThrowSpeed);
    }
}

// ─── Precision Shot ──────────────────────────────────────────────────────────
void ABossEnemy::StartPrecisionCycle()
{
    GetWorldTimerManager().SetTimer(PrecisionCycleHandle, this,
        &ABossEnemy::StartPrecisionAim, PrecisionInterval, true, PrecisionInterval);
}

void ABossEnemy::StartPrecisionAim()
{
    if (bIsDead || CurrentAlertLevel != EAlertLevel::Combat || !IsValid(BossTarget)) return;

    ShowBossAlert(TEXT("INCOMING PRECISION SHOT"), PrecisionAimDuration);

    GetWorldTimerManager().SetTimer(PrecisionFireHandle, this,
        &ABossEnemy::ExecutePrecisionFire, PrecisionAimDuration, false);
}

void ABossEnemy::ExecutePrecisionFire()
{
    if (bIsDead || !IsValid(BossTarget)) return;

    const FName MuzzleSocket = TEXT("Muzzle");
    const FVector AimStart = (WeaponMeshComp && WeaponMeshComp->DoesSocketExist(MuzzleSocket))
        ? WeaponMeshComp->GetSocketLocation(MuzzleSocket)
        : GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);

    const FVector AimDir = (BossTarget->GetActorLocation() - AimStart).GetSafeNormal();

    CombatManagerComp->OnFire(AimStart, AimDir, ECombatWeaponType::Rifle, PrecisionDamage, true);
}

// ─── Reinforcement ───────────────────────────────────────────────────────────
void ABossEnemy::SpawnReinforcement()
{
    if (!ReinforcementClass || bHasSpawnedReinforcement) return;
    bHasSpawnedReinforcement = true;

    ShowBossAlert(TEXT("REINFORCEMENTS INCOMING"), 3.f);

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    for (int32 i = 0; i < ReinforcementCount; ++i)
    {
        const FVector Offset = FVector(
            FMath::RandRange(-400.f, 400.f),
            FMath::RandRange(-400.f, 400.f),
            0.f);

        AEnemyCharacter* Reinforcement = GetWorld()->SpawnActor<AEnemyCharacter>(
            ReinforcementClass,
            GetActorLocation() + Offset,
            FRotator::ZeroRotator,
            SpawnParams);

        if (!Reinforcement || !IsValid(BossTarget)) continue;

        if (AAIController* AIC = Cast<AAIController>(Reinforcement->GetController()))
        {
            if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
            {
                BB->SetValueAsObject(BBKeys::TARGET_ACTOR, BossTarget);
                BB->SetValueAsVector(BBKeys::LAST_KNOWN_LOCATION, BossTarget->GetActorLocation());
            }
        }
        Reinforcement->OnAlertLevelChanged(EAlertLevel::Combat);
    }
}

// ─── Reward ──────────────────────────────────────────────────────────────────
void ABossEnemy::SpawnReward()
{
    if (!RewardClass) return;

    GetWorld()->SpawnActor<AActor>(RewardClass, GetActorLocation(), FRotator::ZeroRotator);
}
