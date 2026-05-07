#include "CCTV.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyCharacter.h"
#include "Engine/World.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/OverlapResult.h"
#include "Components/CapsuleComponent.h"

ACCTV::ACCTV()
{
    CurrentAlertLevel = EAlertLevel::CCTV;
    LastPerceivedTarget = nullptr;

    AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

    if (SightConfig)
    {
        SightConfig->SightRadius = SightRange;
        SightConfig->LoseSightRadius = SightRange + 200.0f;
        // 기획서: FOVAngle 120도 → UE는 반각(Half-Angle) 기준이므로 /2 불필요
        // PeripheralVisionAngleDegrees는 중심에서 한쪽 방향 각도 (Half-FOV)
        SightConfig->PeripheralVisionAngleDegrees = VisualFOV / 2.0f;
        SightConfig->DetectionByAffiliation.bDetectEnemies = true;
        SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
        SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

        AIPerceptionComp->ConfigureSense(*SightConfig);
    }
}

void ACCTV::BeginPlay()
{
    Super::BeginPlay();
    AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &ACCTV::OnTargetPerceived);
}

void ACCTV::Die()
{
    if (bIsDead) return;

    // 파괴되기 직전, 마지막으로 감지한 타겟이 있으면 최고 단계로 경보 전파
    if (LastPerceivedTarget)
    {
        UE_LOG(LogTemp, Warning, TEXT("CCTV 파괴됨: 최종 경보를 전파합니다."));
        AlertNearbyEnemies(LastPerceivedTarget, true);
    }

    // Perception 즉시 비활성화 (파괴 후 감지 이벤트 방지)
    if (AIPerceptionComp)
    {
        AIPerceptionComp->SetSenseEnabled(UAISense_Sight::StaticClass(), false);
        AIPerceptionComp->Deactivate();
    }

    // 콜리전 비활성화 (총알이 더 이상 피격 판정하지 않도록)
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // 메시는 남겨두되 피격 콜리전만 제거 (파괴된 카메라 연출 유지)
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    UE_LOG(LogTemp, Warning, TEXT("CCTV [%s] 파괴 완료. Perception 비활성화."), *GetName());

    // 부모의 Die() 호출 (bIsDead = true, OnDeath 브로드캐스트)
    Super::Die();
}

void ACCTV::OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
    if (bIsDead) return;

    if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            // 감지한 타겟 캐싱 (파괴 시 최종 경보에 사용)
            LastPerceivedTarget = Actor;

            UE_LOG(LogTemp, Warning, TEXT("CCTV [%s]: 플레이어 침입 감지! 주변 동료에게 전파합니다."), *GetName());
            AlertNearbyEnemies(Actor, false);
        }
        else
        {
            // 시야 벗어남: 캐싱 유지 (마지막 위치 기억)
            UE_LOG(LogTemp, Log, TEXT("CCTV [%s]: 타겟을 시야에서 놓쳤습니다."), *GetName());
        }
    }
}

// bForceMaxAlert: true이면 파괴 시 강제 Combat 전파
void ACCTV::AlertNearbyEnemies(AActor* TargetPlayer, bool bForceMaxAlert)
{
    if (!TargetPlayer) return;

    UWorld* World = GetWorld();
    if (!World) return;

    TArray<FOverlapResult> OverlapResults;
    FCollisionShape CollisionSphere = FCollisionShape::MakeSphere(ShareRange);

    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    bool bHasOverlap = World->OverlapMultiByObjectType(
        OverlapResults,
        GetActorLocation(),
        FQuat::Identity,
        ObjectParams,
        CollisionSphere,
        QueryParams
    );

    if (!bHasOverlap) return;

    for (const FOverlapResult& Result : OverlapResults)
    {
        AActor* OverlappedActor = Result.GetActor();
        if (!OverlappedActor) continue;

        AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(OverlappedActor);
        if (!Enemy) continue;

        AAIController* AIC = Cast<AAIController>(Enemy->GetController());
        if (!AIC) continue;

        UBlackboardComponent* BB = AIC->GetBlackboardComponent();
        if (!BB) continue;

        EAlertLevel CurrentEnemyLevel = Enemy->GetCurrentAlertLevel();

        // CCTV 파괴 시: 주변 모든 적을 즉시 Combat으로 강제 전환
        if (bForceMaxAlert)
        {
            Enemy->OnAlertLevelChanged(EAlertLevel::Combat);
            BB->SetValueAsObject(TEXT("TargetActor"), TargetPlayer);
            BB->SetValueAsVector(TEXT("LastKnownLocation"), TargetPlayer->GetActorLocation());

            UE_LOG(LogTemp, Warning, TEXT("CCTV 파괴 경보: [%s] 강제 Combat 전환"), *Enemy->GetName());
            continue;
        }

        // 일반 감지 시: 1단계씩 상승
        if (CurrentEnemyLevel == EAlertLevel::Idle)
        {
            Enemy->OnAlertLevelChanged(EAlertLevel::Suspicious);
            BB->SetValueAsVector(TEXT("LastKnownLocation"), TargetPlayer->GetActorLocation());
            BB->ClearValue(TEXT("TargetActor"));

            UE_LOG(LogTemp, Log, TEXT("CCTV 경보: [%s] Idle → Suspicious"), *Enemy->GetName());
        }
        else if (CurrentEnemyLevel == EAlertLevel::Suspicious)
        {
            Enemy->OnAlertLevelChanged(EAlertLevel::Combat);
            BB->SetValueAsObject(TEXT("TargetActor"), TargetPlayer);
            BB->SetValueAsVector(TEXT("LastKnownLocation"), TargetPlayer->GetActorLocation());

            UE_LOG(LogTemp, Warning, TEXT("CCTV 경보: [%s] Suspicious → Combat"), *Enemy->GetName());
        }
    }
}