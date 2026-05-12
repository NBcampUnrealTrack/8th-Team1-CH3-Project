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
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "BlackboardKeys.h"

ACCTV::ACCTV()
{
    CurrentAlertLevel = EAlertLevel::CCTV;
    BrokenCCTVMesh = nullptr;

    // 스켈레탈 메쉬 숨김 처리
    if (GetMesh())
    {
        GetMesh()->SetVisibility(false);
        GetMesh()->DestroyPhysicsState();
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // 스태틱 메쉬 컴포넌트 추가
    CCTVMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CCTVMeshComp"));
    CCTVMeshComp->SetupAttachment(GetRootComponent());

    AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

    if (SightConfig)
    {
        SightConfig->SightRadius = SightRange;
        SightConfig->LoseSightRadius = SightRange + 200.0f;
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

    // Perception 즉시 비활성화 (파괴 후 감지 이벤트 방지)
    if (AIPerceptionComp)
    {
        AIPerceptionComp->SetSenseEnabled(UAISense_Sight::StaticClass(), false);
        AIPerceptionComp->Deactivate();
    }

    // 부서진 CCTV 메시를 현재 위치/회전으로 스폰
    if (BrokenCCTVMesh)
    {
        UWorld* World = GetWorld();
        if (World)
        {
            AStaticMeshActor* BrokenActor = World->SpawnActor<AStaticMeshActor>(
                AStaticMeshActor::StaticClass(),
                GetActorLocation(),
                GetActorRotation()
            );

            if (BrokenActor)
            {
                BrokenActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
                BrokenActor->GetStaticMeshComponent()->SetStaticMesh(BrokenCCTVMesh);
                UE_LOG(LogTemp, Log, TEXT("CCTV [%s]: 파괴된 메시 스폰 완료."), *GetName());
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("CCTV [%s]: 파괴됨. 경보 전파 없음."), *GetName());

    // bIsDead = true, OnDeath 브로드캐스트
    Super::Die();

    // 원본 액터 제거
    Destroy();
}

void ACCTV::OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
    if (bIsDead) return;

    if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            UE_LOG(LogTemp, Warning, TEXT("CCTV [%s]: 플레이어 감지. 주변 적을 Combat으로 전환합니다."), *GetName());
            AlertNearbyEnemies(Actor);
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("CCTV [%s]: 타겟을 시야에서 놓쳤습니다."), *GetName());
        }
    }
}

// CCTV는 플레이어의 정확한 위치를 알고 있으므로, 주변 적을 즉시 Combat으로 전환
void ACCTV::AlertNearbyEnemies(AActor* TargetPlayer)
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

    const bool bHasOverlap = World->OverlapMultiByObjectType(
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

        // Combat 미만인 적만 갱신 (이미 전투 중인 적은 건드리지 않음)
        if (Enemy->GetCurrentAlertLevel() < EAlertLevel::Combat)
        {
            Enemy->OnAlertLevelChanged(EAlertLevel::Combat);
            BB->SetValueAsObject(TEXT("TargetActor"), TargetPlayer);
            BB->SetValueAsVector(TEXT("LastKnownLocation"), TargetPlayer->GetActorLocation());

            UE_LOG(LogTemp, Warning, TEXT("CCTV 경보: [%s] → Combat 전환"), *Enemy->GetName());
        }
    }
}