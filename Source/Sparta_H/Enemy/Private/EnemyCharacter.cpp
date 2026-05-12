#include "EnemyCharacter.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/OverlapResult.h"

AEnemyCharacter::AEnemyCharacter()
{
    AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    CombatManagerComp = CreateDefaultSubobject<UCombatManager>(TEXT("CombatManager"));

    AlertIconWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("AlertIconWidget"));
    AlertIconWidgetComp->SetupAttachment(GetMesh(), TEXT("head"));
    AlertIconWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
    AlertIconWidgetComp->SetDrawSize(FVector2D(64.f, 64.f));
    AlertIconWidgetComp->SetVisibility(false);

    if (SightConfig)
    {
        SightConfig->DetectionByAffiliation.bDetectEnemies = true;
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

void AEnemyCharacter::OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
    if (bIsDead || !Actor) return; // Actor가 유효한지 확인
    AAIController* AIC = Cast<AAIController>(GetController());
    if (!AIC) return;
    UBlackboardComponent* BB = AIC->GetBlackboardComponent();
    if (!BB) return;

    if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            GetWorldTimerManager().ClearTimer(LostRevertTimerHandle);
            SuspectedTarget = Actor;

            if (CurrentAlertLevel < EAlertLevel::Combat && !GetWorldTimerManager().IsTimerActive(SpotCheckTimerHandle))
            {
                if (CurrentAlertLevel == EAlertLevel::Idle) OnAlertLevelChanged(EAlertLevel::Suspicious);
                
                SpotProb = 0.7f;
                GetWorldTimerManager().SetTimer(SpotCheckTimerHandle, this, &AEnemyCharacter::ProcessSpotCheck, 1.0f, true);
            }
            BB->SetValueAsVector(TEXT("LastKnownLocation"), Stimulus.StimulusLocation);
        }
        else 
        {
            GetWorldTimerManager().ClearTimer(SpotCheckTimerHandle);
            if (CurrentAlertLevel == EAlertLevel::Combat) OnAlertLevelChanged(EAlertLevel::Lost);
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

void AEnemyCharacter::StartFirePattern(AActor* TargetActor)
{
    if (bIsDead || !TargetActor) return;

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

// ---------------------------------------------------------------
// AlertLevel 변경 + Perception + 아이콘 + 타이머 + 주변 적 동기화
// ---------------------------------------------------------------
void AEnemyCharacter::OnAlertLevelChanged(EAlertLevel NewLevel)
{
    if (bIsDead) return;

    // 상태 전환 로그 출력 (어떤 상태에서 어떤 상태로 바뀌는지 확인 가능)
    FString EnumName = StaticEnum<EAlertLevel>()->GetNameStringByValue((int64)NewLevel);
    UE_LOG(LogTemp, Warning, TEXT("[%s] 경계 레벨 변경: %s"), *GetName(), *EnumName);

    GetWorldTimerManager().ClearTimer(SuspiciousRevertTimerHandle);
    GetWorldTimerManager().ClearTimer(LostRevertTimerHandle);

    Super::OnAlertLevelChanged(NewLevel);

    switch (NewLevel)
    {
    case EAlertLevel::Idle:       ApplyPerceptionStats(IdleStats);       break;
    case EAlertLevel::Suspicious: ApplyPerceptionStats(SuspiciousStats); break;
    case EAlertLevel::Combat:     ApplyPerceptionStats(CombatStats);     break;
    case EAlertLevel::Lost:       ApplyPerceptionStats(LostStats);       break;
    default: break;
    }

    UpdateAlertIcon(NewLevel);

    switch (NewLevel)
    {
    case EAlertLevel::Suspicious:
        GetWorldTimerManager().SetTimer(SuspiciousRevertTimerHandle, this, &AEnemyCharacter::OnSuspiciousRevertTimerExpired, SuspiciousRevertDelay, false);
        break;

    case EAlertLevel::Combat:
        if (SuspectedTarget) StartFirePattern(SuspectedTarget);
        AlertNearbyEnemies(SuspectedTarget, CombatAlertRange, EAlertLevel::Combat);
        break;

    case EAlertLevel::Lost:
        AlertNearbyEnemies(SuspectedTarget, LostAlertRange, EAlertLevel::Suspicious);
        // --- TEST용: Lost 대기 시간 없이 즉시 Idle로 복귀 ---
        //            GetWorldTimerManager().SetTimer(LostRevertTimerHandle, this, &AEnemyCharacter::OnLostRevertTimerExpired, LostRevertDelay, false);
        OnLostRevertTimerExpired(); 
        break;
    default: break;
    }
}

void AEnemyCharacter::ApplyPerceptionStats(const FAlertLevelStats& Stats)
{
    // --- 포커스 제어 로직 추가 ---
    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        if (CurrentAlertLevel == EAlertLevel::Combat && SuspectedTarget)
        {
            // 전투 중일 때는 타겟을 고정해서 바라봄 (시야각 확보)
            AIC->SetFocus(SuspectedTarget);
        }
        else
        {
            // 평시나 타겟을 놓쳤을 때는 시선 고정 해제
            AIC->ClearFocus(EAIFocusPriority::Gameplay);
        }
    }
    
    if (SightConfig && AIPerceptionComp)
    {
        SightConfig->SightRadius = Stats.SightRange;
        SightConfig->LoseSightRadius = Stats.SightRange + 50.f; // 여유 거리를 더 늘림
        
        // 언리얼 Perception FOV는 절반 값입니다. 
        // 입력받은 FOVAngle이 90이면 좌우 45도씩 총 90도를 봅니다.
        SightConfig->PeripheralVisionAngleDegrees = Stats.FOVAngle / 2.0f;
        
        // 중요: 변경된 설정을 재등록
        AIPerceptionComp->ConfigureSense(*SightConfig);
    }

    if (HearingConfig && AIPerceptionComp)
    {
        HearingConfig->HearingRange = Stats.HearingRange;
        AIPerceptionComp->ConfigureSense(*HearingConfig);
    }

    // 런타임에 Perception 시스템에 변경 사항 알림
    AIPerceptionComp->RequestStimuliListenerUpdate();

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = Stats.MoveSpeed;
    }
}

void AEnemyCharacter::UpdateAlertIcon(EAlertLevel NewLevel)
{
    if (!AlertIconWidgetComp) return;
    UEnemyAlertWidget* AlertWidget = Cast<UEnemyAlertWidget>(AlertIconWidgetComp->GetUserWidgetObject());
    if (AlertWidget) AlertWidget->OnAlertLevelUpdated(NewLevel);

    if (NewLevel == EAlertLevel::Suspicious || NewLevel == EAlertLevel::Combat)
    {
        AlertIconWidgetComp->SetVisibility(true);
        GetWorldTimerManager().SetTimer(IconHideTimerHandle, this, &AEnemyCharacter::HideAlertIcon, IconHideDelay, false);
    }
    else
    {
        AlertIconWidgetComp->SetVisibility(false);
    }
}

void AEnemyCharacter::HideAlertIcon() { if (AlertIconWidgetComp) AlertIconWidgetComp->SetVisibility(false); }

void AEnemyCharacter::AlertNearbyEnemies(AActor* TargetPlayer, float AlertRange, EAlertLevel NewLevel)
{
    if (!TargetPlayer) return;

    TArray<FOverlapResult> OverlapResults;
    // ◀ 에러 해결: InitWithAllObjects 대신 기본 생성자 사용
    FCollisionObjectQueryParams ObjectParams; 
    ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

    GetWorld()->OverlapMultiByObjectType(OverlapResults, GetActorLocation(), FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(AlertRange));

    for (auto& Result : OverlapResults)
    {
        AEnemyCharacter* NearbyEnemy = Cast<AEnemyCharacter>(Result.GetActor());
        if (NearbyEnemy && NearbyEnemy != this && !NearbyEnemy->bIsDead)
        {
            if (NearbyEnemy->GetCurrentAlertLevel() < NewLevel) NearbyEnemy->OnAlertLevelChanged(NewLevel);
        }
    }
}

void AEnemyCharacter::OnSuspiciousRevertTimerExpired() { OnAlertLevelChanged(EAlertLevel::Idle); }
void AEnemyCharacter::OnLostRevertTimerExpired() { OnAlertLevelChanged(EAlertLevel::Idle); }

bool AEnemyCharacter::CanShootTarget(AActor* TargetActor)
{
    if (!TargetActor) return false;
    float Dist = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
    if (Dist > FireRange) return false;

    FVector Dir = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(GetActorForwardVector(), Dir)));
    
    return Angle <= FireAngleLimit;
}

bool AEnemyCharacter::FireAtTarget(AActor* TargetActor)
{
    if (!TargetActor || !CombatManagerComp) return false;

    bool bIsHit = FMath::FRand() <= HitAccuracy;
    FVector MuzzleLoc = GetMesh()->GetSocketLocation(TEXT("MuzzleSocket"));
    if (MuzzleLoc.IsZero()) MuzzleLoc = GetActorLocation() + FVector(0,0,70);
    FVector FireDir = (TargetActor->GetActorLocation() - MuzzleLoc).GetSafeNormal();

    CombatManagerComp->OnFire(MuzzleLoc, FireDir, ECombatWeaponType::Rifle, WeaponDamage, 0.f);
    return bIsHit;
}