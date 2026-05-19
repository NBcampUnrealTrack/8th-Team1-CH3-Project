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
    // 메시 임포트 방향이 액터 Forward(X축)와 90도 어긋나는 경우 보정
    // 반대 방향이면 -90.f → 90.f 로 변경
    GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

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
            // "PatrolLocationA", "PatrolLocationB"는 블랙보드에 생성한 Vector 키 이름입니다.
            BB->SetValueAsVector(BBKeys::LOCATION_A, PatrolWorldLocationA);
            BB->SetValueAsVector(BBKeys::LOCATION_B, PatrolWorldLocationB);
        }
    }
    
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->bUseRVOAvoidance = true;
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
        UE_LOG(LogTemp, Warning, TEXT("[%s] 플레이어 발각 확정!"), *GetName());
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
    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        if (CurrentAlertLevel == EAlertLevel::Combat && SuspectedTarget) AIC->SetFocus(SuspectedTarget);
        else AIC->ClearFocus(EAIFocusPriority::Gameplay);
    }
    
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

    // 1. C++ 내부 변수 업데이트
    CurrentAlertLevel = NewLevel;

    // 2. 블랙보드에 실시간 기록 (이게 없으면 BT가 작동 안 함)
    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
        {
            // BBKeys::ALERT_LEVEL을 사용하여 비헤이비어 트리에 신호 전송
            BB->SetValueAsEnum(BBKeys::ALERT_LEVEL, (uint8)NewLevel);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[%s] 경계 레벨 변경: %d"), *GetName(), (int32)NewLevel);
    
    // 타이머 초기화
    GetWorldTimerManager().ClearTimer(SuspiciousRevertTimerHandle);

    Super::OnAlertLevelChanged(NewLevel);

    // 상태별 스탯/아이콘 적용
    switch (NewLevel)
    {
        case EAlertLevel::Idle:       ApplyPerceptionStats(IdleStats);       break;
        case EAlertLevel::Suspicious: ApplyPerceptionStats(SuspiciousStats); break;
        case EAlertLevel::Combat:     ApplyPerceptionStats(CombatStats);     break;
        case EAlertLevel::Lost:       ApplyPerceptionStats(LostStats);       break;
        default: break;
    }

    UpdateAlertIcon(NewLevel);

    // 타겟 정보 가져오기
    AActor* TargetPlayer = nullptr;
    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
            TargetPlayer = Cast<AActor>(BB->GetValueAsObject(BBKeys::TARGET_ACTOR));
    }

    // 상태별 타이머 및 알람 전파
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
    UBlackboardComponent* BB = Cast<AAIController>(GetController())->GetBlackboardComponent();
    if (!BB) return;

    if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
    {
        if (Stimulus.WasSuccessfullySensed()) // 플레이어를 보았을 때
        {
            SuspectedTarget = Actor;
            BB->SetValueAsObject(BBKeys::TARGET_ACTOR, Actor);
            BB->SetValueAsVector(BBKeys::LAST_KNOWN_LOCATION, Actor->GetActorLocation());

            // 2. 경계 레벨 올리기
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
        else
        {
            //-------------------------------------------
            // ClearValue시 바로 lost되는 문제 발생가능
            //-------------------------------------------

            GetWorldTimerManager().ClearTimer(SpotCheckTimerHandle);

            if (CurrentAlertLevel == EAlertLevel::Combat)
            {
                OnAlertLevelChanged(EAlertLevel::Lost);
            }
        }
    }
    else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            UE_LOG(LogTemp, Warning, TEXT("[%s] 청각 감지! 소음 위치=%s | 현재 경계=%d"),
                *GetName(), *Stimulus.StimulusLocation.ToString(), (int32)CurrentAlertLevel);

            // 소음 위치를 마지막 알려진 위치로 기록 → BT가 조사하러 이동
            BB->SetValueAsVector(BBKeys::LAST_KNOWN_LOCATION, Stimulus.StimulusLocation);

            // Idle이면 Suspicious로 전환, Suspicious 이상이면 Combat으로 전환
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

    if (CurrentShotCount == 0 && !GetWorldTimerManager().IsTimerActive(FirePatternTimerHandle))
    {
        SuspectedTarget = TargetActor;
        UE_LOG(LogTemp, Warning, TEXT("[%s] 사격 패턴 시작! 타겟: %s"), *GetName(), *TargetActor->GetName());
        
        ExecuteFireStep();
    }
}

void AEnemyCharacter::ExecuteFireStep()
{
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
        }
    }
    else
    {
        CurrentShotCount = 0;
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

    // 이동만 멈추고 Unpossess는 몽타주 시작 후로 미룸
    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        AIC->StopMovement();
    }

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->DisableMovement();
    }

    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        AIC->UnPossess();
    }

    UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
    if (DeathMontage && AnimInst)
    {
        const float MontageDuration = AnimInst->Montage_Play(DeathMontage, 1.0f);
        const float DestroyDelay = FMath::Max(MontageDuration - 0.2f, 0.1f);

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

    const FVector DirectionToTarget = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    const float AngleToTarget = FMath::RadiansToDegrees(
        FMath::Acos(FVector::DotProduct(GetActorForwardVector(), DirectionToTarget))
    );
    if (AngleToTarget > FireAngleLimit) return false;

    FHitResult HitResult;
    FCollisionQueryParams CollisionParams;
    CollisionParams.AddIgnoredActor(this);

    const FName MuzzleSocket = TEXT("Muzzle");
    FVector StartLocation = (WeaponMeshComp && WeaponMeshComp->DoesSocketExist(MuzzleSocket))
        ? WeaponMeshComp->GetSocketLocation(MuzzleSocket)
        : GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);

    const bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult, StartLocation, TargetActor->GetActorLocation(),
        ECC_Visibility, CollisionParams
    );

    return bHit && HitResult.GetActor() == TargetActor;
}

bool AEnemyCharacter::FireAtTarget(AActor* TargetActor)
{
    if (bIsDead || !TargetActor || !CombatManagerComp) return false;

    if (!CanShootTarget(TargetActor)) return false;

    const float RandomRoll = FMath::FRand();
    const bool bIsHit = RandomRoll <= HitAccuracy;

    if (!bIsHit) return false;

    const FName MuzzleSocket = TEXT("Muzzle");
    FVector AimStart = (WeaponMeshComp && WeaponMeshComp->DoesSocketExist(MuzzleSocket))
        ? WeaponMeshComp->GetSocketLocation(MuzzleSocket)
        : GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
    const FVector AimDirection = (TargetActor->GetActorLocation() - AimStart).GetSafeNormal();

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