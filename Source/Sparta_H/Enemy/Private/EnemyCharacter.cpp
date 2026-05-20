#include "EnemyCharacter.h"
#include "DrawDebugHelpers.h" // 💡 [추가] 라인 트레이스를 화면에 그리기 위한 필수 헤더
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

AEnemyCharacter::AEnemyCharacter()
{
    GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

    AIPerceptionComp  = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
    SightConfig       = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    HearingConfig     = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
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

    AlertIconWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, AlertIconHeightOffset));
    
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

// ---------------------------------------------------------------
// Perception 수치 런타임 갱신
// ---------------------------------------------------------------
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
            BB->SetValueAsEnum(BBKeys::ALERT_LEVEL, (uint8)NewLevel);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[%s] 경계 레벨 변경: %d"), *GetName(), (int32)NewLevel);
    GetWorldTimerManager().ClearTimer(SuspiciousRevertTimerHandle);

    Super::OnAlertLevelChanged(NewLevel);

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->bOrientRotationToMovement = (NewLevel != EAlertLevel::Combat);
    }

    switch (NewLevel)
    {
        case EAlertLevel::Idle:       ApplyPerceptionStats(IdleStats);       break;
        case EAlertLevel::Suspicious: ApplyPerceptionStats(SuspiciousStats); break;
        case EAlertLevel::Combat:     ApplyPerceptionStats(CombatStats);     break;
        case EAlertLevel::Lost:       ApplyPerceptionStats(LostStats);       break;
        default: break;
    }

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
        if (AAlertManager* AlertMgr = AAlertManager::GetInstance(this)) AlertMgr->NotifyCombatEntered(GetActorLocation());
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

            if (CurrentAlertLevel == EAlertLevel::Lost)
            {
                UE_LOG(LogTemp, Warning, TEXT("[%s] 4단계(Lost) 상태에서 플레이어 재발견! 3단계(Combat)로 복귀합니다."), *GetName());
                OnAlertLevelChanged(EAlertLevel::Combat);
                return;
            }

            // 1, 2단계일 경우 경계도 올리기
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
        // 청각 감지 로직 기존 코드 유지
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
    if (TargetActor->GetName().Contains(TEXT("Hostage"))) return;

    if (GetWorldTimerManager().IsTimerActive(FirePatternTimerHandle)) return;
    if (CurrentShotCount != 0) return;

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

    UE_LOG(LogTemp, Warning, TEXT("[%s] 사격 패턴 시작! 타겟 액터: %s"), *GetName(), *TargetActor->GetName());
    
    ExecuteFireStep();
}

void AEnemyCharacter::ExecuteFireStep()
{
    FString TargetName = SuspectedTarget ? SuspectedTarget->GetName() : TEXT("None");

    if (bIsDead || CurrentAlertLevel != EAlertLevel::Combat || !SuspectedTarget)
    {
        CurrentShotCount = 0;
        if (AAIController* AIC = Cast<AAIController>(GetController())) AIC->ClearFocus(EAIFocusPriority::Gameplay);
        if (UCharacterMovementComponent* MoveComp = GetCharacterMovement()) MoveComp->bOrientRotationToMovement = true;
        return;
    }

    if (CurrentShotCount < 3)
    {
        UE_LOG(LogTemp, Log, TEXT("[%s] 점사 진행 중: 사격 조건을 검사합니다. 검사 대상: %s"), *GetName(), *TargetName);

        if (CanShootTarget(SuspectedTarget))
        {
            FireAtTarget(SuspectedTarget);
            CurrentShotCount++;
            
            UE_LOG(LogTemp, Log, TEXT("[%s] 발사 단계 성공. 0.4초 후 다음 탄 발사를 예약합니다. 대상: %s"), *GetName(), *TargetName);
            GetWorldTimerManager().SetTimer(FirePatternTimerHandle, this, &AEnemyCharacter::ExecuteFireStep, 0.4f, false);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[%s] 점사 도중 사격 조건을 상실하여 루틴을 강제 종료합니다. 대상: %s"), *GetName(), *TargetName);
            CurrentShotCount = 0;
            if (AAIController* AIC = Cast<AAIController>(GetController())) AIC->ClearFocus(EAIFocusPriority::Gameplay);
            if (UCharacterMovementComponent* MoveComp = GetCharacterMovement()) MoveComp->bOrientRotationToMovement = true;
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[%s] 3점사 발사 완료. 0.8초간 사격 쿨다운 제어에 들어갑니다. 대상: %s"), *GetName(), *TargetName);
        CurrentShotCount = 0;
        
        if (AAIController* AIC = Cast<AAIController>(GetController())) AIC->ClearFocus(EAIFocusPriority::Gameplay);
        if (UCharacterMovementComponent* MoveComp = GetCharacterMovement()) MoveComp->bOrientRotationToMovement = true;

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

void AEnemyCharacter::Die()
{
    Super::Die();

    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (AAIController* AIC = Cast<AAIController>(GetController())) AIC->StopMovement();
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement()) MoveComp->DisableMovement();
    if (AAIController* AIC = Cast<AAIController>(GetController())) AIC->UnPossess();

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

// ---------------------------------------------------------------
// 사격 가시성 및 트레이스 디버그 라인 표현
// ---------------------------------------------------------------
bool AEnemyCharacter::CanShootTarget(AActor* TargetActor)
{
    if (!TargetActor) return false;

    // 1. 거리 제한 검사
    const float Distance = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
    if (Distance > FireRange)
    {
        UE_LOG(LogTemp, Log, TEXT("[%s] CanShoot FAIL — 거리 초과 (%.0f / %.0f)"), *GetName(), Distance, FireRange);
        return false;
    }

    // 2. 시야각 제한 검사
    const FVector DirectionToTarget = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    const float AngleToTarget = FMath::RadiansToDegrees(
        FMath::Acos(FVector::DotProduct(GetActorForwardVector(), DirectionToTarget))
    );
    if (AngleToTarget > FireAngleLimit)
    {
        UE_LOG(LogTemp, Log, TEXT("[%s] CanShoot FAIL — 각도 초과 (%.1f° / %.1f°)"), *GetName(), AngleToTarget, FireAngleLimit);
        return false;
    }

    // 3. 라인 트레이스 세팅
    FHitResult HitResult;
    FCollisionQueryParams CollisionParams;
    
    // 자기 자신(몸, 무기, 캡슐)은 무시
    CollisionParams.AddIgnoredActor(this);
    if (WeaponMeshComp) CollisionParams.AddIgnoredComponent(WeaponMeshComp);
    if (GetCapsuleComponent()) CollisionParams.AddIgnoredComponent(GetCapsuleComponent());

    // 4. 발사 시작 지점(총구)
    const FName MuzzleSocket = TEXT("Muzzle");
    FVector StartLocation = (WeaponMeshComp && WeaponMeshComp->DoesSocketExist(MuzzleSocket))
        ? WeaponMeshComp->GetSocketLocation(MuzzleSocket) : GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);

<<<<<<< HEAD
    // 5. 조준점(타겟) 보정: 플레이어 캡슐 정중앙으로 계산
    FVector TargetLocation = TargetActor->GetActorLocation();
    if (ACharacter* TargetChar = Cast<ACharacter>(TargetActor))
    {
        TargetLocation += FVector(0.f, 0.f, TargetChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.7f);
    }
    else
    {
        TargetLocation += FVector(0.f, 0.f, 80.f);
    }

    // 6. 💡 ECC_Pawn 채널을 사용 (아까 설정한 Pawn 콜리전 채널)
    const bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult, StartLocation, TargetLocation,
        ECC_Pawn, 
        CollisionParams
    );

    // 7. 디버그 라인: 초록색(성공), 빨간색(충돌)
    DrawDebugLine(GetWorld(), StartLocation, bHit ? HitResult.ImpactPoint : TargetLocation, 
                  bHit ? FColor::Red : FColor::Green, false, 2.0f, 0, 2.0f);

    // 8. 명중 판정 (트레이스가 타겟에 닿았는지 확인)
    bool bCanShoot = bHit && (HitResult.GetActor() == TargetActor);

    UE_LOG(LogTemp, Log, TEXT("[%s] CanShootTarget 결과: %d | 적중 액터: %s"), 
           *GetName(), bCanShoot, HitResult.GetActor() ? *HitResult.GetActor()->GetName() : TEXT("None"));

    return bCanShoot;
=======
    const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
        HitResult, StartLocation, TargetActor->GetActorLocation(),
        ECC_Visibility, CollisionParams
    );

    // 아무것도 막히지 않으면 시야 확보 → 사격 가능
    // 뭔가 막혔는데 타겟이 아니면(벽 등) → 사격 불가
    if (bBlocked && HitResult.GetActor() != TargetActor)
    {
        UE_LOG(LogTemp, Log, TEXT("[%s] CanShoot FAIL — 장애물에 막힘 (맞은 것: %s)"),
            *GetName(), *HitResult.GetActor()->GetName());
        return false;
    }

    return true;
>>>>>>> Dev
}

bool AEnemyCharacter::FireAtTarget(AActor* TargetActor)
{
    if (bIsDead || !TargetActor || !CombatManagerComp) return false;

    const FName MuzzleSocket = TEXT("Muzzle");
    FVector AimStart = (WeaponMeshComp && WeaponMeshComp->DoesSocketExist(MuzzleSocket))
<<<<<<< HEAD
        ? WeaponMeshComp->GetSocketLocation(MuzzleSocket) : GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
        
    FVector FinalTargetLocation = TargetActor->GetActorLocation();
    if (ACharacter* TargetChar = Cast<ACharacter>(TargetActor))
=======
        ? WeaponMeshComp->GetSocketLocation(MuzzleSocket)
        : GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
    const FVector AimDirection = (TargetActor->GetActorLocation() + FVector(0.f, 0.f, 80.f) - AimStart).GetSafeNormal();

    if (FireMontage)
>>>>>>> Dev
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

    CombatManagerComp->OnFire(AimStart, AimDirection, ECombatWeaponType::Rifle, WeaponDamage, true);

    UE_LOG(LogTemp, Warning, TEXT("[%s] FireAtTarget 발사! 타겟: %s, 실제 적중여부: %d"), *GetName(), *TargetActor->GetName(), bCanHit);

    return true;
}

void AEnemyCharacter::OnCombatToLostTimerExpired()
{
    if (bIsDead || CurrentAlertLevel != EAlertLevel::Combat) return;

    UE_LOG(LogTemp, Warning, TEXT("[%s] 플레이어를 2초간 놓쳐 4단계(Lost) 상태로 전환합니다."), *GetName());
    OnAlertLevelChanged(EAlertLevel::Lost);
}