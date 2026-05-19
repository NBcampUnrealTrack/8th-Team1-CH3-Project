#include "EnemyCharacter.h"
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

AEnemyCharacter::AEnemyCharacter()
{
    AIPerceptionComp  = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
    SightConfig       = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    HearingConfig     = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    CombatManagerComp = CreateDefaultSubobject<UCombatManager>(TEXT("CombatManager"));

    WeaponMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    WeaponMeshComp->SetupAttachment(GetMesh(), TEXT("GripPoint"));

    // 머리 위 아이콘 위젯 컴포넌트 생성
    AlertIconWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("AlertIconWidget"));
    AlertIconWidgetComp->SetupAttachment(GetMesh(), TEXT("head"));
    AlertIconWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
    AlertIconWidgetComp->SetDrawSize(FVector2D(64.f, 64.f));
    AlertIconWidgetComp->SetVisibility(false);

    if (SightConfig)
    {
        SightConfig->DetectionByAffiliation.bDetectEnemies    = true;
        SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
        SightConfig->DetectionByAffiliation.bDetectNeutrals   = true;
        AIPerceptionComp->ConfigureSense(*SightConfig);
    }

    if (HearingConfig)
    {
        HearingConfig->HearingRange = IdleStats.HearingRange;
        HearingConfig->DetectionByAffiliation.bDetectEnemies    = true;
        HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
        HearingConfig->DetectionByAffiliation.bDetectNeutrals   = true;
        AIPerceptionComp->ConfigureSense(*HearingConfig);
    }

    AIPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());
    
    bUseControllerRotationYaw = true; 
}

void AEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();
    AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyCharacter::OnTargetPerceived);
    ApplyPerceptionStats(IdleStats);
    
    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        if (EnemyBT)
        {
            AIC->RunBehaviorTree(EnemyBT);
        }
        // 리스폰/스폰 시 블랙보드에 에디터에서 지정한 A, B 절대 좌표를 그대로 주입
        if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
        {
            BB->SetValueAsVector(BBKeys::LOCATION_A, PatrolWorldLocationA);
            BB->SetValueAsVector(BBKeys::LOCATION_B, PatrolWorldLocationB);
        }
    }
    
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->bUseRVOAvoidance = true;
        MoveComp->bOrientRotationToMovement = true; 
    }
}

// ---------------------------------------------------------------
// 타입별 스탯 초기화
// ---------------------------------------------------------------
void AEnemyCharacter::InitializeStats()
{
    Super::InitializeStats();
    WeaponDamage = Damage;
}

// 1초마다 확률 검사 (기획: Visibility * SpotProb >= Rand)
void AEnemyCharacter::ProcessSpotCheck()
{
    if (!SuspectedTarget)
    {
        GetWorldTimerManager().ClearTimer(SpotCheckTimerHandle);
        return;
    }

    // 플레이어를 보고 있는 동안 확률이 더 빨리 올라가도록 보정
    float FinalChance = SpotProb;
    float RandomValue = FMath::FRand();

    if (FinalChance >= RandomValue)
    {
        OnAlertLevelChanged(EAlertLevel::Combat);
        GetWorldTimerManager().ClearTimer(SpotCheckTimerHandle);
    }
    else
    {
        // 1초마다 0.15씩 증가 (약 2~3초 안에 확정 발견되도록)
        SpotProb += 0.15f; 
    }
}

// ---------------------------------------------------------------
// 머리 위 아이콘 갱신
// ---------------------------------------------------------------
void AEnemyCharacter::UpdateAlertIcon(EAlertLevel NewLevel)
{
    if (!AlertIconWidgetComp) return;

    // Idle / CCTV / Lost : 즉시 숨김 + 타이머 취소
    if (NewLevel == EAlertLevel::Idle   ||
        NewLevel == EAlertLevel::CCTV   ||
        NewLevel == EAlertLevel::Lost)
    {
        GetWorldTimerManager().ClearTimer(IconHideTimerHandle);
        AlertIconWidgetComp->SetVisibility(false);
        return;
    }

    // Suspicious(??) / Combat(!!) : 아이콘 표시 후 IconHideDelay초 뒤 숨김
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

// ---------------------------------------------------------------
// Perception 수치 런타임 갱신
// ---------------------------------------------------------------
void AEnemyCharacter::ApplyPerceptionStats(const FAlertLevelStats& Stats)
{
    /*if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        if (CurrentAlertLevel == EAlertLevel::Combat && SuspectedTarget) AIC->SetFocus(SuspectedTarget);
        else AIC->ClearFocus(EAIFocusPriority::Gameplay);
    }*/
    
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

// ---------------------------------------------------------------
// AlertLevel 변경 + Perception + 아이콘 + 타이머 + 주변 적 동기화
// ---------------------------------------------------------------
void AEnemyCharacter::OnAlertLevelChanged(EAlertLevel NewLevel)
{
    if (bIsDead) return;

    CurrentAlertLevel = NewLevel;

    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
        {
            BB->SetValueAsEnum(BBKeys::ALERT_LEVEL, (uint8)NewLevel);
        }
    }
    
    // 타이머 일괄 정리 (상태가 바뀔 때 이전 상태의 타이머가 도는 것을 방지)
    GetWorldTimerManager().ClearTimer(SuspiciousRevertTimerHandle);
    GetWorldTimerManager().ClearTimer(CombatToLostTimerHandle);

    Super::OnAlertLevelChanged(NewLevel);

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        if (NewLevel == EAlertLevel::Combat)
        {
            MoveComp->bOrientRotationToMovement = false;
        }
        else
        {
            MoveComp->bOrientRotationToMovement = true;
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

    UpdateAlertIcon(NewLevel);

    AActor* TargetPlayer = nullptr;
    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
            TargetPlayer = Cast<AActor>(BB->GetValueAsObject(BBKeys::TARGET_ACTOR));
    }

    // 상태별 후처리
    switch (NewLevel)
    {
    case EAlertLevel::Suspicious:
        GetWorldTimerManager().SetTimer(SuspiciousRevertTimerHandle, this, &AEnemyCharacter::OnSuspiciousRevertTimerExpired, SuspiciousRevertDelay, false);
        break;
    case EAlertLevel::Combat:
        if (TargetPlayer) AlertNearbyEnemies(TargetPlayer, CombatAlertRange, EAlertLevel::Combat);

        if (AAlertManager* AlertMgr = AAlertManager::GetInstance(this))
        {
            AlertMgr->NotifyCombatEntered(GetActorLocation());
        }
        break;
    case EAlertLevel::Lost:
        if (TargetPlayer) AlertNearbyEnemies(TargetPlayer, LostAlertRange, EAlertLevel::Suspicious);
        break;
    default: break;
    }
}

// ---------------------------------------------------------------
// 주변 적 동기화
// ---------------------------------------------------------------
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

// ---------------------------------------------------------------
// 자동 복귀 타이머 콜백
// ---------------------------------------------------------------
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

// ---------------------------------------------------------------
// Perception 콜백
// ---------------------------------------------------------------
void AEnemyCharacter::OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
    if (bIsDead || !Actor || Actor == this || !Actor->ActorHasTag(TEXT("Player"))) return;
    
    AAIController* AIC = Cast<AAIController>(GetController());
    if (!AIC) return;
    UBlackboardComponent* BB = AIC->GetBlackboardComponent();
    if (!BB) return;

    if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
    {
        if (Stimulus.WasSuccessfullySensed()) // 플레이어를 보았을 때
        {
            SuspectedTarget = Actor;
            BB->SetValueAsObject(BBKeys::TARGET_ACTOR, Actor);
            BB->SetValueAsVector(BBKeys::LAST_KNOWN_LOCATION, Actor->GetActorLocation());

            if (CurrentAlertLevel == EAlertLevel::Lost)
            {
                OnAlertLevelChanged(EAlertLevel::Combat);
                return;
            }

            GetWorldTimerManager().ClearTimer(CombatToLostTimerHandle);

            if (CurrentAlertLevel < EAlertLevel::Combat)
            {
                OnAlertLevelChanged(EAlertLevel::Suspicious);
                if (!GetWorldTimerManager().IsTimerActive(SpotCheckTimerHandle))
                {
                    SpotProb = 0.7f;
                    GetWorldTimerManager().SetTimer(SpotCheckTimerHandle, this, &AEnemyCharacter::ProcessSpotCheck, 1.0f, true);
                }
            }
        }
        else // 플레이어가 시야에서 사라졌을 때
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
        // 기존 청각 로직 유지
        if (Stimulus.WasSuccessfullySensed())
        {
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
    if (bIsDead || !TargetActor) return;

    if (GetWorldTimerManager().IsTimerActive(FirePatternTimerHandle) || CurrentShotCount != 0) return;

    SuspectedTarget = TargetActor;

    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        AIC->StopMovement();
        AIC->SetFocus(TargetActor, EAIFocusPriority::Gameplay);
    }

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->bOrientRotationToMovement = false;
    }
    
    ExecuteFireStep();
}

void AEnemyCharacter::ExecuteFireStep()
{
    FString TargetName = SuspectedTarget ? SuspectedTarget->GetName() : TEXT("None");

    if (bIsDead || CurrentAlertLevel != EAlertLevel::Combat || !SuspectedTarget)
    {
        CurrentShotCount = 0;
        return;
    }

    if (CurrentShotCount < 3)
    {
        if (CanShootTarget(SuspectedTarget))
        {
            FireAtTarget(SuspectedTarget);
            CurrentShotCount++;
            GetWorldTimerManager().SetTimer(FirePatternTimerHandle, this, &AEnemyCharacter::ExecuteFireStep, 0.4f, false);
        }
        else
        {
            CurrentShotCount = 0;
            
            if (AAIController* AIC = Cast<AAIController>(GetController())) AIC->ClearFocus(EAIFocusPriority::Gameplay);
            if (GetCharacterMovement()) GetCharacterMovement()->bOrientRotationToMovement = true;
        }
    }
    else
    {
        CurrentShotCount = 0;
        
        if (AAIController* AIC = Cast<AAIController>(GetController())) AIC->ClearFocus(EAIFocusPriority::Gameplay);
        if (GetCharacterMovement()) GetCharacterMovement()->bOrientRotationToMovement = true;

        GetWorldTimerManager().SetTimer(FirePatternTimerHandle, 0.8f, false);
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

// ---------------------------------------------------------------
// 사망 처리
// ---------------------------------------------------------------
void AEnemyCharacter::Die()
{
    Super::Die();

    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        AIC->StopMovement();
        AIC->UnPossess();
    }

    if (DeathMontage)
    {
        const float MontageDuration = PlayAnimMontage(DeathMontage);
        const float DestroyDelay = MontageDuration > 0.f ? MontageDuration : 2.f;
        FTimerHandle DestroyTimerHandle;
        GetWorldTimerManager().SetTimer(DestroyTimerHandle, [this]()
        {
            Destroy();
        }, DestroyDelay, false);
    }
    else
    {
        Destroy();
    }
}

// ---------------------------------------------------------------
// 사격 관련
// ---------------------------------------------------------------
bool AEnemyCharacter::CanShootTarget(AActor* TargetActor)
{
    if (!TargetActor) return false;

    const float Distance = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
    if (Distance > FireRange) return false;

    const FVector DirectionToTarget = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    const float AngleToTarget = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(GetActorForwardVector(), DirectionToTarget)));
    if (AngleToTarget > FireAngleLimit) return false;

    FHitResult HitResult;
    FCollisionQueryParams CollisionParams;
    
    CollisionParams.AddIgnoredActor(this); 
    if (WeaponMeshComp) CollisionParams.AddIgnoredComponent(WeaponMeshComp); 
    if (GetCapsuleComponent()) CollisionParams.AddIgnoredComponent(GetCapsuleComponent()); 

    const FName MuzzleSocket = TEXT("Muzzle");
    FVector StartLocation = (WeaponMeshComp && WeaponMeshComp->DoesSocketExist(MuzzleSocket))
        ? WeaponMeshComp->GetSocketLocation(MuzzleSocket)
        : GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);

    FVector TargetLocation = TargetActor->GetActorLocation();
    if (ACharacter* TargetChar = Cast<ACharacter>(TargetActor))
    {
        TargetLocation = TargetChar->GetMesh()->GetSocketLocation(TEXT("spine_03"));
    }
    else
    {
        TargetLocation += FVector(0.f, 0.f, 90.f);
    }

    const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, TargetLocation, ECC_Visibility, CollisionParams);

    //DrawDebugLine(GetWorld(), StartLocation, bHit ? HitResult.ImpactPoint : TargetLocation, bHit ? FColor::Red : FColor::Green, false, 0.2f, 0, 2.0f);

    if (!bHit || HitResult.GetActor() == TargetActor || HitResult.GetActor()->ActorHasTag(TEXT("Player")) || HitResult.GetActor()->IsA(AEnemyCharacter::StaticClass()))
    {
        return true;
    }
    
    return false;
}

bool AEnemyCharacter::FireAtTarget(AActor* TargetActor)
{
    if (bIsDead || !TargetActor || !CombatManagerComp) return false;

    if (!CanShootTarget(TargetActor)) return false;
    
    const FName MuzzleSocket = TEXT("Muzzle");
    FVector AimStart = (WeaponMeshComp && WeaponMeshComp->DoesSocketExist(MuzzleSocket))
        ? WeaponMeshComp->GetSocketLocation(MuzzleSocket)
        : GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
        
    FVector TargetTargetLocation = TargetActor->GetActorLocation();
    if (ACharacter* TargetChar = Cast<ACharacter>(TargetActor))
    {
        TargetTargetLocation = TargetChar->GetMesh()->GetSocketLocation(TEXT("spine_03"));
    }
    else
    {
        TargetTargetLocation += FVector(0.f, 0.f, 90.f);
    }
    
    const FVector AimDirection = (TargetTargetLocation - AimStart).GetSafeNormal();

    if (FireMontage)
    {
        PlayAnimMontage(FireMontage);
    }

    if (MuzzleFlashEffect && WeaponMeshComp)
    {
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            MuzzleFlashEffect, WeaponMeshComp, TEXT("Muzzle"),
            FVector::ZeroVector, FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget, true);
    }

    CombatManagerComp->OnFire(AimStart, AimDirection, ECombatWeaponType::Rifle, WeaponDamage, true);

    return true;
}

void AEnemyCharacter::OnCombatToLostTimerExpired()
{
    if (bIsDead || CurrentAlertLevel != EAlertLevel::Combat) return;

    UE_LOG(LogTemp, Warning, TEXT("[%s] 플레이어를 20초간 놓쳐 Lost 상태로 전환합니다."), *GetName());
    OnAlertLevelChanged(EAlertLevel::Lost);
}