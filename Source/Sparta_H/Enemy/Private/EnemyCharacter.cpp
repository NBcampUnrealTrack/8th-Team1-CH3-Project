#include "EnemyCharacter.h"
#include "DrawDebugHelpers.h" 
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "AlertManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/OverlapResult.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimMontage.h"
#include "BlackboardKeys.h"
#include "CombatManager.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

AEnemyCharacter::AEnemyCharacter()
{
    GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

    AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    CombatManagerComp = CreateDefaultSubobject<UCombatManager>(TEXT("CombatManager"));

    WeaponMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    WeaponMeshComp->SetupAttachment(GetMesh(), TEXT("GripPoint"));

    AlertIconWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("AlertIconWidget"));
    AlertIconWidgetComp->SetupAttachment(GetRootComponent());
    AlertIconWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
    AlertIconWidgetComp->SetDrawSize(FVector2D(64.f, 64.f));
    AlertIconWidgetComp->SetVisibility(false);

    if (SightConfig)
    {
       SightConfig->DetectionByAffiliation.bDetectEnemies = true;
       SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
       SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
       AIPerceptionComp->ConfigureSense(*SightConfig);
    }

    if (HearingConfig)
    {
       HearingConfig->HearingRange = IdleStats.HearingRange;
       HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
       HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
       HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
       AIPerceptionComp->ConfigureSense(*HearingConfig);
    }

    AIPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());
    
    // 기본 상태에서는 Yaw 강제 고정을 끕니다. (MovementComponent가 회전을 제어하도록 위임)
    bUseControllerRotationYaw = false; 
}

void AEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (bUseRandomPatrol)
    {
        auto GetRandomOffset = []() -> FVector {
            float X = FMath::FRandRange(50.f, 150.f) * (FMath::RandBool() ? 1.f : -1.f);
            float Y = FMath::FRandRange(50.f, 150.f) * (FMath::RandBool() ? 1.f : -1.f);
            return FVector(X, Y, 0.f);
        };

        PatrolOffsetA = GetRandomOffset();
        PatrolOffsetB = GetRandomOffset();
    }
    
    PatrolSpawnLocation = GetActorLocation();
    
    AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyCharacter::OnTargetPerceived);
    ApplyPerceptionStats(IdleStats);

    AlertIconWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, AlertIconHeightOffset));
    
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->bUseRVOAvoidance = true;
        MoveComp->AvoidanceConsiderationRadius = 110.0f;
        MoveComp->bOrientRotationToMovement = true;
    }
    
    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
        {
            BB->SetValueAsVector(TEXT("PatrolLocationA"), GetPatrolWorldLocationA());
            BB->SetValueAsVector(TEXT("PatrolLocationB"), GetPatrolWorldLocationB());
        }
    }
}

void AEnemyCharacter::InitializeStats()
{
    Super::InitializeStats();
    WeaponDamage = Damage;
}

void AEnemyCharacter::ProcessSpotCheck()
{
    if (!SuspectedTarget)
    {
        GetWorldTimerManager().ClearTimer(SpotCheckTimerHandle);
        return;
    }

    float FinalChance = SpotProb;
    float RandomValue = FMath::FRand();

    if (FinalChance >= RandomValue)
    {
        OnAlertLevelChanged(EAlertLevel::Combat);
        GetWorldTimerManager().ClearTimer(SpotCheckTimerHandle);
        UE_LOG(LogTemp, Warning, TEXT("[%s] 플레이어 발각 확정! 타겟: %s"), *GetName(), *SuspectedTarget->GetName());
    }
    else
    {
        SpotProb += 0.15f; 
    }
}

void AEnemyCharacter::UpdateAlertIcon(EAlertLevel NewLevel)
{
    if (!AlertIconWidgetComp) return;

    if (NewLevel == EAlertLevel::Idle   ||
        NewLevel == EAlertLevel::CCTV   ||
        NewLevel == EAlertLevel::Lost)
    {
        GetWorldTimerManager().ClearTimer(IconHideTimerHandle);
        AlertIconWidgetComp->SetVisibility(false);
        return;
    }

    UEnemyAlertWidget* AlertWidget = Cast<UEnemyAlertWidget>(AlertIconWidgetComp->GetUserWidgetObject());
    if (AlertWidget)
    {
        AlertWidget->OnAlertLevelUpdated(NewLevel);
    }

    AlertIconWidgetComp->SetVisibility(true);

    GetWorldTimerManager().SetTimer(
        IconHideTimerHandle, this,
        &AEnemyCharacter::HideAlertIcon,
        IconHideDelay, false
    );
}

void AEnemyCharacter::HideAlertIcon()
{
    if (AlertIconWidgetComp)
    {
        AlertIconWidgetComp->SetVisibility(false);
    }
}

void AEnemyCharacter::ShowExclamationIcon(float Duration)
{
    if (!AlertIconWidgetComp) return;

    GetWorldTimerManager().ClearTimer(IconHideTimerHandle);

    UEnemyAlertWidget* AlertWidget = Cast<UEnemyAlertWidget>(AlertIconWidgetComp->GetUserWidgetObject());
    if (AlertWidget)
    {
        AlertWidget->OnAlertLevelUpdated(EAlertLevel::Combat);
    }

    AlertIconWidgetComp->SetVisibility(true);
    GetWorldTimerManager().SetTimer(IconHideTimerHandle, this, &AEnemyCharacter::HideAlertIcon, Duration, false);
}

void AEnemyCharacter::ApplyPerceptionStats(const FAlertLevelStats& Stats)
{
    if (SightConfig)
    {
        SightConfig->SightRadius = Stats.SightRange;
        SightConfig->LoseSightRadius = Stats.SightRange;
        SightConfig->PeripheralVisionAngleDegrees = Stats.FOVAngle / 2.0f;
        AIPerceptionComp->ConfigureSense(*SightConfig);
    }

    if (HearingConfig)
    {
        HearingConfig->HearingRange = Stats.HearingRange;
        AIPerceptionComp->ConfigureSense(*HearingConfig);
    }

    if (GetCharacterMovement()) GetCharacterMovement()->MaxWalkSpeed = Stats.MoveSpeed;
    AIPerceptionComp->RequestStimuliListenerUpdate();
}

void AEnemyCharacter::OnAlertLevelChanged(EAlertLevel NewLevel)
{
    if (bIsDead) return;
    if (CurrentAlertLevel == NewLevel) return;

    CurrentAlertLevel = NewLevel;

    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
        {
            BB->SetValueAsEnum(BBKeys::ALERT_LEVEL, (uint8)NewLevel);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[%s] 경계 레벨 변경: %d"), *GetName(), (int32)NewLevel);
    GetWorldTimerManager().ClearTimer(SuspiciousRevertTimerHandle);
    GetWorldTimerManager().ClearTimer(RepositionTimerHandle);
    bIsRepositioning = false;

    Super::OnAlertLevelChanged(NewLevel);

    AActor* TargetPlayer = nullptr;
    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
            TargetPlayer = Cast<AActor>(BB->GetValueAsObject(BBKeys::TARGET_ACTOR));
    }

    // 💡 [핵심 픽스] 전투 진입 시 포커스와 회전 동기화를 이 곳에서 통제합니다.
    bool bIsCombat = (NewLevel == EAlertLevel::Combat);
    
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->bOrientRotationToMovement = !bIsCombat; // 전투 중엔 이동 방향을 보지 않음 (게걸음)
        MoveComp->bUseControllerDesiredRotation = bIsCombat; // 컨트롤러의 시선(포커스)을 몸이 부드럽게 따라감
        
        if (bIsCombat)
        {
            // 몸을 빠르게 틀 수 있도록 회전 속도를 확 올려줍니다.
            MoveComp->RotationRate = FRotator(0.0f, 800.0f, 0.0f);
        }
    }

    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        if (bIsCombat && TargetPlayer)
        {
            AIC->SetFocus(TargetPlayer, EAIFocusPriority::Gameplay);
        }
        else
        {
            AIC->ClearFocus(EAIFocusPriority::Gameplay);
        }
    }

    switch (NewLevel)
    {
        case EAlertLevel::Idle:       ApplyPerceptionStats(IdleStats);       break;
        case EAlertLevel::Suspicious: ApplyPerceptionStats(SuspiciousStats); break;
        case EAlertLevel::Combat:     ApplyPerceptionStats(CombatStats);     break;
        case EAlertLevel::Lost:       ApplyPerceptionStats(LostStats);       break;
        default: break;
    }

    switch (NewLevel)
    {
    case EAlertLevel::Suspicious:
        GetWorldTimerManager().SetTimer(SuspiciousRevertTimerHandle, this, &AEnemyCharacter::OnSuspiciousRevertTimerExpired, SuspiciousRevertDelay, false);
        break;
    case EAlertLevel::Combat:
        if (TargetPlayer) AlertNearbyEnemies(TargetPlayer, CombatAlertRange, EAlertLevel::Combat);
        if (AAlertManager* AlertMgr = AAlertManager::GetInstance(this)) AlertMgr->NotifyCombatEntered(GetActorLocation());
        break;
    case EAlertLevel::Lost:
        if (AAIController* AIC = Cast<AAIController>(GetController()))
        {
            if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
            {
                BB->ClearValue(BBKeys::TARGET_ACTOR);
            }
        }
        if (TargetPlayer) AlertNearbyEnemies(TargetPlayer, LostAlertRange, EAlertLevel::Suspicious);
        break;
    default: break;
    }
}

void AEnemyCharacter::AlertNearbyEnemies(AActor* TargetPlayer, float AlertRange, EAlertLevel NewLevel)
{
    if (!TargetPlayer) return;

    UWorld* World = GetWorld();
    if (!World) return;

    TArray<FOverlapResult> OverlapResults;
    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    World->OverlapMultiByObjectType(
        OverlapResults,
        GetActorLocation(),
        FQuat::Identity,
        ObjectParams,
        FCollisionShape::MakeSphere(AlertRange),
        QueryParams
    );

    for (const FOverlapResult& Result : OverlapResults)
    {
        AEnemyCharacter* NearbyEnemy = Cast<AEnemyCharacter>(Result.GetActor());
        if (!NearbyEnemy || NearbyEnemy->IsDead()) continue;
        if (NearbyEnemy->GetCurrentAlertLevel() >= NewLevel) continue;

        AAIController* AIC = Cast<AAIController>(NearbyEnemy->GetController());
        if (!AIC) continue;
        UBlackboardComponent* BB = AIC->GetBlackboardComponent();
        if (!BB) continue;

        NearbyEnemy->OnAlertLevelChanged(NewLevel);
        BB->SetValueAsVector(BBKeys::LAST_KNOWN_LOCATION, TargetPlayer->GetActorLocation());

        if (NewLevel == EAlertLevel::Combat)
        {
            BB->SetValueAsObject(BBKeys::TARGET_ACTOR, TargetPlayer);
        }
    }
}

void AEnemyCharacter::OnSuspiciousRevertTimerExpired()
{
    if (CurrentAlertLevel != EAlertLevel::Suspicious) return;

    OnAlertLevelChanged(EAlertLevel::Idle);

    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
        {
            BB->ClearValue(BBKeys::LAST_KNOWN_LOCATION);
            BB->ClearValue(BBKeys::TARGET_ACTOR);
        }
    }
}

void AEnemyCharacter::OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
    if (bIsDead || !Actor || Actor == this) return;
    if (!Actor->ActorHasTag(TEXT("Player")) || Actor->GetName().Contains(TEXT("Hostage"))) return;
    
    AAIController* AIC = Cast<AAIController>(GetController());
    if (!AIC) return;
    UBlackboardComponent* BB = AIC->GetBlackboardComponent();
    if (!BB) return;

    if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            SuspectedTarget = Actor;
            BB->SetValueAsObject(BBKeys::TARGET_ACTOR, Actor);
            BB->SetValueAsVector(BBKeys::LAST_KNOWN_LOCATION, Actor->GetActorLocation());

            GetWorldTimerManager().ClearTimer(CombatToLostTimerHandle);

            if (CurrentAlertLevel == EAlertLevel::Lost || CurrentAlertLevel < EAlertLevel::Combat)
            {
                if (CurrentAlertLevel == EAlertLevel::Lost)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[%s] 4단계(Lost) 상태에서 플레이어 포착! (확률 검사 시작)"), *GetName());
                }
                
                OnAlertLevelChanged(EAlertLevel::Suspicious);
                
                if (!GetWorldTimerManager().IsTimerActive(SpotCheckTimerHandle))
                {
                    SpotProb = 0.7f;
                    GetWorldTimerManager().SetTimer(SpotCheckTimerHandle, this, &AEnemyCharacter::ProcessSpotCheck, 1.0f, true);
                }
            }
        }
        else 
        {
            GetWorldTimerManager().ClearTimer(SpotCheckTimerHandle);

            if (CurrentAlertLevel == EAlertLevel::Combat)
            {
                GetWorldTimerManager().SetTimer(
                    CombatToLostTimerHandle, 
                    this, 
                    &AEnemyCharacter::OnCombatToLostTimerExpired, 
                    2.0f, 
                    false
                );
            }
        }
    }
    else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            UE_LOG(LogTemp, Warning, TEXT("[%s] 청각 감지! 소음 위치=%s | 현재 경계=%d"),
                *GetName(), *Stimulus.StimulusLocation.ToString(), (int32)CurrentAlertLevel);

            BB->SetValueAsVector(BBKeys::LAST_KNOWN_LOCATION, Stimulus.StimulusLocation);

            if (CurrentAlertLevel == EAlertLevel::Idle)
            {
                OnAlertLevelChanged(EAlertLevel::Suspicious);
            }
            else if (CurrentAlertLevel == EAlertLevel::Suspicious)
            {
                SuspectedTarget = Actor;
                BB->SetValueAsObject(BBKeys::TARGET_ACTOR, Actor);
                OnAlertLevelChanged(EAlertLevel::Combat);
            }
        }
    }
}

void AEnemyCharacter::StartFirePattern(AActor* TargetActor)
{
    if (bIsDead || !TargetActor || TargetActor == this) return;
    if (TargetActor->GetName().Contains(TEXT("Hostage"))) return;
    if (TargetActor->ActorHasTag(TEXT("Enemy"))) return;

    if (GetWorldTimerManager().IsTimerActive(FirePatternTimerHandle)) return;
    if (CurrentShotCount != 0) return;
    if (bIsRepositioning) return;

    SuspectedTarget = TargetActor;

    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        AIC->StopMovement();
        // 시선 관리는 OnAlertLevelChanged에서 하지만, 확실히 하기 위해 다시 강제합니다.
        AIC->SetFocus(TargetActor, EAIFocusPriority::Gameplay);
    }

    UE_LOG(LogTemp, Warning, TEXT("[%s] 사격 패턴 시작! 타겟 액터: %s"), *GetName(), *TargetActor->GetName());
    
    ExecuteFireStep();
}

void AEnemyCharacter::ExecuteFireStep()
{
    FString TargetName = SuspectedTarget ? SuspectedTarget->GetName() : TEXT("None");

    if (bIsDead || CurrentAlertLevel != EAlertLevel::Combat || !SuspectedTarget
        || SuspectedTarget->ActorHasTag(TEXT("Enemy")))
    {
        CurrentShotCount = 0;
        SuspectedTarget = nullptr;
        // 전투 상태가 끝났을 때만 OnAlertLevelChanged 로직에 의해 포커스가 해제됩니다. 여기선 건드리지 않습니다.
        return;
    }

    if (CurrentShotCount < 3)
    {
        UE_LOG(LogTemp, Log, TEXT("[%s] 점사 진행 중: 사격 조건을 검사합니다. 검사 대상: %s"), *GetName(), *TargetName);

        AActor* Blocker = nullptr;
        if (CanShootTarget(SuspectedTarget, &Blocker))
        {
            FireAtTarget(SuspectedTarget);
            CurrentShotCount++;

            UE_LOG(LogTemp, Log, TEXT("[%s] 발사 단계 성공. 0.4초 후 다음 탄 발사를 예약합니다. 대상: %s"), *GetName(), *TargetName);
            GetWorldTimerManager().SetTimer(FirePatternTimerHandle, this, &AEnemyCharacter::ExecuteFireStep, 0.4f, false);
        }
        else 
        {
            // 💡 [핵심 픽스] 각도 검사 분리: 못 쏜 이유가 각도(덜 돌아서) 때문인지 확인합니다.
            const FVector DirToTarget = (SuspectedTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
            const float AngleToTarget = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(GetActorForwardVector(), DirToTarget)));
            
            if (AngleToTarget > FireAngleLimit)
            {
                // 타겟이 시야각 밖에 있다면 사격을 취소하지 않고 0.1초 뒤 다시 체크합니다. (돌아볼 시간을 줌)
                UE_LOG(LogTemp, Log, TEXT("[%s] 타겟이 시야각(%.1f도) 밖에 있어 사격 보류 및 몸 회전 대기 중..."), *GetName(), AngleToTarget);
                GetWorldTimerManager().SetTimer(FirePatternTimerHandle, this, &AEnemyCharacter::ExecuteFireStep, 0.1f, false);
            }
            else if (Blocker && Blocker->ActorHasTag(TEXT("Enemy")))
            {
                UE_LOG(LogTemp, Warning, TEXT("[%s] 아군(%s)에 막혀 재배치 시작"), *GetName(), *Blocker->GetName());
                CurrentShotCount = 0;
                bIsRepositioning = true;

                if (AAIController* AIC = Cast<AAIController>(GetController()))
                {
                    AIC->MoveToActor(SuspectedTarget, 600.f);
                }

                GetWorldTimerManager().SetTimer(RepositionTimerHandle, this, &AEnemyCharacter::TryRepositionForShot, 0.5f, false);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[%s] 점사 도중 사격 조건을 상실하여 사격을 종료합니다. 대상: %s"), *GetName(), *TargetName);
                CurrentShotCount = 0;
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[%s] 3점사 발사 완료. 0.8초간 사격 쿨다운 제어에 들어갑니다. 대상: %s"), *GetName(), *TargetName);
        CurrentShotCount = 0;
        GetWorldTimerManager().SetTimer(FirePatternTimerHandle, 0.8f, false);
    }
}

void AEnemyCharacter::TryRepositionForShot()
{
    if (bIsDead || CurrentAlertLevel != EAlertLevel::Combat || !SuspectedTarget)
    {
        bIsRepositioning = false;
        return;
    }

    AActor* Blocker = nullptr;
    if (CanShootTarget(SuspectedTarget, &Blocker))
    {
        bIsRepositioning = false;

        if (AAIController* AIC = Cast<AAIController>(GetController()))
        {
            AIC->StopMovement();
        }

        UE_LOG(LogTemp, Warning, TEXT("[%s] 시야 확보 완료 — 사격 재개"), *GetName());
        ExecuteFireStep();
    }
    else if (Blocker && Blocker->ActorHasTag(TEXT("Enemy")))
    {
        if (AAIController* AIC = Cast<AAIController>(GetController()))
            AIC->MoveToActor(SuspectedTarget, 600.f);

        GetWorldTimerManager().SetTimer(RepositionTimerHandle, this, &AEnemyCharacter::TryRepositionForShot, 0.5f, false);
    }
    else
    {
        bIsRepositioning = false;
    }
}

void AEnemyCharacter::OnDetectionTimerExpired()
{
    if (bIsDead || !SuspectedTarget) return;

    AAIController* AIC = Cast<AAIController>(GetController());
    if (!AIC) return;
    UBlackboardComponent* BB = AIC->GetBlackboardComponent();
    if (!BB) return;

    OnAlertLevelChanged(EAlertLevel::Combat);
    BB->SetValueAsObject(BBKeys::TARGET_ACTOR, SuspectedTarget);
    SuspectedTarget = nullptr;
}

void AEnemyCharacter::Die()
{
    Super::Die();

    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetWorldTimerManager().ClearTimer(RepositionTimerHandle);
    bIsRepositioning = false;

    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        AIC->ClearFocus(EAIFocusPriority::Gameplay);
        AIC->StopMovement();
        AIC->UnPossess();
    }
    
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement()) MoveComp->DisableMovement();

    UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
    if (DeathMontage && AnimInst)
    {
        const float MontageDuration = AnimInst->Montage_Play(DeathMontage, 1.0f);
        const float DestroyDelay = FMath::Max(MontageDuration - 0.2f, 0.1f);

        FTimerHandle DestroyTimerHandle;
        GetWorldTimerManager().SetTimer(DestroyTimerHandle, [this](){ Destroy(); }, DestroyDelay, false);
    }
    else
    {
        Destroy();
    }
}

bool AEnemyCharacter::CanShootTarget(AActor* TargetActor, AActor** OutBlocker)
{
    if (!TargetActor) return false;

    if (OutBlocker) *OutBlocker = nullptr;

    const float Distance = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
    if (Distance > FireRange)
    {
        UE_LOG(LogTemp, Log, TEXT("[%s] CanShoot FAIL — 거리 초과 (%.0f / %.0f)"), *GetName(), Distance, FireRange);
        return false;
    }

    const FVector DirectionToTarget = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    const float AngleToTarget = FMath::RadiansToDegrees(
        FMath::Acos(FVector::DotProduct(GetActorForwardVector(), DirectionToTarget))
    );
    if (AngleToTarget > FireAngleLimit)
    {
        UE_LOG(LogTemp, Log, TEXT("[%s] CanShoot FAIL — 각도 초과 (%.1f° / %.1f°)"), *GetName(), AngleToTarget, FireAngleLimit);
        return false;
    }

    FHitResult HitResult;
    FCollisionQueryParams CollisionParams;
    
    CollisionParams.AddIgnoredActor(this);
    if (WeaponMeshComp) CollisionParams.AddIgnoredComponent(WeaponMeshComp);
    if (GetCapsuleComponent()) CollisionParams.AddIgnoredComponent(GetCapsuleComponent());

    const FName MuzzleSocket = TEXT("Muzzle");
    FVector StartLocation = (WeaponMeshComp && WeaponMeshComp->DoesSocketExist(MuzzleSocket))
        ? WeaponMeshComp->GetSocketLocation(MuzzleSocket) : GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);

    FVector TargetLocation = TargetActor->GetActorLocation();
    if (ACharacter* TargetChar = Cast<ACharacter>(TargetActor))
    {
        TargetLocation += FVector(0.f, 0.f, TargetChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.7f);
    }
    else
    {
        TargetLocation += FVector(0.f, 0.f, 80.f);
    }

    const bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult, StartLocation, TargetLocation,
        ECC_Pawn, 
        CollisionParams
    );

    DrawDebugLine(GetWorld(), StartLocation, bHit ? HitResult.ImpactPoint : TargetLocation, 
                  bHit ? FColor::Red : FColor::Green, false, 2.0f, 0, 2.0f);

    bool bCanShoot = bHit && (HitResult.GetActor() == TargetActor);

    if (!bCanShoot && OutBlocker)
        *OutBlocker = HitResult.GetActor();

    UE_LOG(LogTemp, Log, TEXT("[%s] CanShootTarget 결과: %d | 적중 액터: %s"),
           *GetName(), bCanShoot, HitResult.GetActor() ? *HitResult.GetActor()->GetName() : TEXT("None"));

    return bCanShoot;
}

bool AEnemyCharacter::FireAtTarget(AActor* TargetActor)
{
    if (bIsDead || !TargetActor || !CombatManagerComp) return false;

    const FName MuzzleSocket = TEXT("Muzzle");
    FVector AimStart = (WeaponMeshComp && WeaponMeshComp->DoesSocketExist(MuzzleSocket))
        ? WeaponMeshComp->GetSocketLocation(MuzzleSocket) : GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
        
    FVector FinalTargetLocation = TargetActor->GetActorLocation();
    if (ACharacter* TargetChar = Cast<ACharacter>(TargetActor))
    {
        FinalTargetLocation.Z += (TargetChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.7f);
    }
    else
    {
        FinalTargetLocation.Z += 80.f;
    }
    
    const FVector AimDirection = (FinalTargetLocation - AimStart).GetSafeNormal();

    FHitResult TestHit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    
    bool bCanHit = GetWorld()->LineTraceSingleByChannel(TestHit, AimStart, FinalTargetLocation, ECC_Pawn, Params);

    DrawDebugLine(GetWorld(), AimStart, bCanHit ? TestHit.ImpactPoint : FinalTargetLocation, 
                  bCanHit ? FColor::Red : FColor::Green, false, 2.0f, 0, 2.0f);

    if (FireMontage) PlayAnimMontage(FireMontage);
    if (MuzzleFlashEffect && WeaponMeshComp) {
        UNiagaraFunctionLibrary::SpawnSystemAttached(MuzzleFlashEffect, WeaponMeshComp, TEXT("Muzzle"), FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
    }

    // CombatManagerComp->OnFire(AimStart, AimDirection, ECombatWeaponType::Rifle, WeaponDamage, true);

    CombatManagerComp->OnFire(AimStart, AimDirection, ECombatWeaponType::Rifle, 1, true);

    UE_LOG(LogTemp, Warning, TEXT("[%s] FireAtTarget 발사! 타겟: %s, 실제 적중여부: %d"), *GetName(), *TargetActor->GetName(), bCanHit);

    return true;
}

void AEnemyCharacter::OnCombatToLostTimerExpired()
{
    if (bIsDead || CurrentAlertLevel != EAlertLevel::Combat) return;

    UE_LOG(LogTemp, Warning, TEXT("[%s] 플레이어를 2초간 놓쳐 4단계(Lost) 상태로 전환합니다."), *GetName());
    
    SuspectedTarget = nullptr;
    
    OnAlertLevelChanged(EAlertLevel::Lost);
}

void AEnemyCharacter::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    
    if (GetWorld() && GetWorld()->WorldType == EWorldType::Editor)
    {
        FVector Origin = GetActorLocation();
        DrawDebugSphere(GetWorld(), Origin + PatrolOffsetA, 30.f, 12, FColor::Green, false, -1.f, 0, 2.f);
        DrawDebugSphere(GetWorld(), Origin + PatrolOffsetB, 30.f, 12, FColor::Red, false, -1.f, 0, 2.f);
    }
}