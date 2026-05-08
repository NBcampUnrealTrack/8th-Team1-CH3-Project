#include "EnemyCharacter.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "BlackboardKeys.h"

AEnemyCharacter::AEnemyCharacter()
{
    AIPerceptionComp  = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
    SightConfig       = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    HearingConfig     = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    CombatManagerComp = CreateDefaultSubobject<UCombatManager>(TEXT("CombatManager"));

    // 머리 위 아이콘 위젯 컴포넌트 생성
    AlertIconWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("AlertIconWidget"));
    AlertIconWidgetComp->SetupAttachment(GetMesh(), TEXT("head"));   // head 소켓에 부착
    AlertIconWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);        // 항상 카메라를 향하도록
    AlertIconWidgetComp->SetDrawSize(FVector2D(64.f, 64.f));
    AlertIconWidgetComp->SetVisibility(false);                        // 초기엔 숨김

    if (SightConfig)
    {
        SightConfig->SightRadius = IdleStats.SightRange;
        SightConfig->LoseSightRadius = IdleStats.SightRange + 300.0f;
        SightConfig->PeripheralVisionAngleDegrees = IdleStats.FOVAngle / 2.0f;
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
}

// ---------------------------------------------------------------
// 타입별 스탯 초기화
// ---------------------------------------------------------------
void AEnemyCharacter::InitializeStats()
{
    Super::InitializeStats(); // EnemyStatData → MaxHealth, CurrentHealth, Damage 세팅

    // BaseEnemy::Damage를 EnemyCharacter::WeaponDamage에 동기화
    WeaponDamage = Damage;
}

// ---------------------------------------------------------------
// 머리 위 아이콘 갱신
// ---------------------------------------------------------------
void AEnemyCharacter::UpdateAlertIcon(EAlertLevel NewLevel)
{
    if (!AlertIconWidgetComp) return;

    // Idle이면 아이콘 숨김
    if (NewLevel == EAlertLevel::Idle || NewLevel == EAlertLevel::CCTV)
    {
        AlertIconWidgetComp->SetVisibility(false);
        return;
    }

    // 위젯이 할당돼 있으면 상태 전달
    UEnemyAlertWidget* AlertWidget = Cast<UEnemyAlertWidget>(AlertIconWidgetComp->GetUserWidgetObject());
    if (AlertWidget)
    {
        AlertWidget->OnAlertLevelUpdated(NewLevel);
    }

    AlertIconWidgetComp->SetVisibility(true);
}

// ---------------------------------------------------------------
// Perception 수치 런타임 갱신
// ---------------------------------------------------------------
void AEnemyCharacter::ApplyPerceptionStats(const FAlertLevelStats& Stats)
{
    if (SightConfig)
    {
        SightConfig->SightRadius = Stats.SightRange;
        SightConfig->LoseSightRadius = Stats.SightRange + 300.0f;
        SightConfig->PeripheralVisionAngleDegrees = Stats.FOVAngle / 2.0f;
        AIPerceptionComp->ConfigureSense(*SightConfig);
    }

    if (HearingConfig)
    {
        HearingConfig->HearingRange = Stats.HearingRange;
        AIPerceptionComp->ConfigureSense(*HearingConfig);
    }

    AIPerceptionComp->RequestStimuliListenerUpdate();

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = Stats.MoveSpeed;
    }
}

// ---------------------------------------------------------------
// AlertLevel 변경 + Perception + 아이콘 + 타이머 + 주변 적 동기화
// ---------------------------------------------------------------
void AEnemyCharacter::OnAlertLevelChanged(EAlertLevel NewLevel)
{
    if (bIsDead) return;

    GetWorldTimerManager().ClearTimer(SuspiciousRevertTimerHandle);
    GetWorldTimerManager().ClearTimer(LostRevertTimerHandle);

    Super::OnAlertLevelChanged(NewLevel);

    // Perception 수치 갱신
    switch (NewLevel)
    {
    case EAlertLevel::Idle:       ApplyPerceptionStats(IdleStats);       break;
    case EAlertLevel::Suspicious: ApplyPerceptionStats(SuspiciousStats); break;
    case EAlertLevel::Combat:     ApplyPerceptionStats(CombatStats);     break;
    case EAlertLevel::Lost:       ApplyPerceptionStats(LostStats);       break;
    default: break;
    }

    // 머리 위 아이콘 갱신
    UpdateAlertIcon(NewLevel);

    // 블랙보드 타겟 액터 참조
    AActor* TargetPlayer = nullptr;
    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
        {
            TargetPlayer = Cast<AActor>(BB->GetValueAsObject(TEXT("TargetActor")));
        }
    }

    switch (NewLevel)
    {
    case EAlertLevel::Suspicious:
        GetWorldTimerManager().SetTimer(
            SuspiciousRevertTimerHandle, this,
            &AEnemyCharacter::OnSuspiciousRevertTimerExpired,
            SuspiciousRevertDelay, false
        );
        break;

    case EAlertLevel::Combat:
        if (TargetPlayer)
        {
            AlertNearbyEnemies(TargetPlayer, CombatAlertRange, EAlertLevel::Combat);
        }
        break;

    case EAlertLevel::Lost:
        if (TargetPlayer)
        {
            AlertNearbyEnemies(TargetPlayer, LostAlertRange, EAlertLevel::Suspicious);
        }
        GetWorldTimerManager().SetTimer(
            LostRevertTimerHandle, this,
            &AEnemyCharacter::OnLostRevertTimerExpired,
            LostRevertDelay, false
        );
        break;

    default:
        break;
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

void AEnemyCharacter::OnLostRevertTimerExpired()
{
    if (CurrentAlertLevel != EAlertLevel::Lost) return;

    OnAlertLevelChanged(EAlertLevel::Idle);

    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
        {
            BB->ClearValue(TEXT("LastKnownLocation"));
            BB->ClearValue(TEXT("TargetActor"));
        }
    }
}

// ---------------------------------------------------------------
// Perception 콜백
// ---------------------------------------------------------------
void AEnemyCharacter::OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
    if (bIsDead) return;

    AAIController* AIC = Cast<AAIController>(GetController());
    if (!AIC) return;
    UBlackboardComponent* BB = AIC->GetBlackboardComponent();
    if (!BB) return;

    if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            if (!GetWorldTimerManager().IsTimerActive(DetectionTimerHandle) && CurrentAlertLevel < EAlertLevel::Combat)
            {
                SuspectedTarget = Actor;
                GetWorldTimerManager().SetTimer(
                    DetectionTimerHandle, this,
                    &AEnemyCharacter::OnDetectionTimerExpired,
                    1.0f, false
                );
                BB->SetValueAsVector(TEXT("LastKnownLocation"), Stimulus.StimulusLocation);
            }
        }
        else
        {
            if (GetWorldTimerManager().IsTimerActive(DetectionTimerHandle))
            {
                GetWorldTimerManager().ClearTimer(DetectionTimerHandle);
                SuspectedTarget = nullptr;
            }

            if (CurrentAlertLevel == EAlertLevel::Combat)
            {
                OnAlertLevelChanged(EAlertLevel::Lost);
            }
        }
    }
    else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
    {
        if (CurrentAlertLevel < EAlertLevel::Suspicious && Stimulus.WasSuccessfullySensed())
        {
            OnAlertLevelChanged(EAlertLevel::Suspicious);
            BB->SetValueAsVector(TEXT("LastKnownLocation"), Stimulus.StimulusLocation);
        }
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
    BB->SetValueAsObject(TEXT("TargetActor"), SuspectedTarget);
    SuspectedTarget = nullptr;
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

    FVector StartLocation = GetMesh()->GetSocketLocation(TEXT("MuzzleSocket"));
    if (StartLocation.IsZero())
    {
        StartLocation = GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
    }

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

    FVector AimStart = GetMesh()->GetSocketLocation(TEXT("MuzzleSocket"));
    if (AimStart.IsZero())
    {
        AimStart = GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
    }
    const FVector AimDirection = (TargetActor->GetActorLocation() - AimStart).GetSafeNormal();

    CombatManagerComp->OnFire(AimStart, AimDirection, ECombatWeaponType::Rifle, WeaponDamage, 0.f);

    return true;
}