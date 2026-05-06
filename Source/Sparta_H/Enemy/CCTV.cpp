#include "CCTV.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "EnemyCharacter.h" 
#include "Engine/World.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/OverlapResult.h"

ACCTV::ACCTV()
{
    CurrentAlertLevel = EAlertLevel::CCTV;

    AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

    if (SightConfig)
    {
        SightConfig->SightRadius = SightRange;
        SightConfig->LoseSightRadius = SightRange + 200.0f;
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

void ACCTV::OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
    if (bIsDead) return;

    if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            UE_LOG(LogTemp, Warning, TEXT("CCTV: 플레이어 침입 감지! 주변 동료들에게 전파합니다."));
            
            AlertNearbyEnemies(Actor);
        }
    }
}

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

    bool bHasOverlap = World->OverlapMultiByObjectType(
        OverlapResults,
        GetActorLocation(),
        FQuat::Identity,
        ObjectParams,
        CollisionSphere,
        QueryParams
    );

    if (bHasOverlap)
    {
        for (const FOverlapResult& Result : OverlapResults)
        {
            AActor* OverlappedActor = Result.GetActor();
            if (OverlappedActor)
            {
                AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(OverlappedActor);
                if (Enemy)
                {
                    AAIController* AIC = Cast<AAIController>(Enemy->GetController());
                    if (AIC)
                    {
                        UBlackboardComponent* BB = AIC->GetBlackboardComponent();
                        if (BB)
                        {
                            // 현재 적의 경계 단계에 따라 정확히 1단계씩 위로 올립니다.
                            EAlertLevel CurrentEnemyLevel = Enemy->GetCurrentAlertLevel();

                            // 1. 적이 완전히 평화(Idle) 상태인 경우 -> 의심(Suspicious)으로 1단계 상승
                            if (CurrentEnemyLevel == EAlertLevel::Idle)
                            {
                                Enemy->OnAlertLevelChanged(EAlertLevel::Suspicious);
                                
                                // 마지막 발견 위치를 알려주어 해당 위치로 수색 이동하도록 유도
                                BB->SetValueAsVector(TEXT("LastKnownLocation"), TargetPlayer->GetActorLocation());
                                BB->ClearValue(TEXT("TargetActor")); // 아직 직접 본 건 아니므로 타겟 액터는 비워둠

                                UE_LOG(LogTemp, Log, TEXT("CCTV 경보 전파: %s가 Idle -> Suspicious(의심) 상태로 승격되었습니다."), *Enemy->GetName());
                            }
                            else if (CurrentEnemyLevel == EAlertLevel::Suspicious)
                            {
                                Enemy->OnAlertLevelChanged(EAlertLevel::Combat);
                                
                                BB->SetValueAsObject(TEXT("TargetActor"), TargetPlayer);
                                BB->SetValueAsVector(TEXT("LastKnownLocation"), TargetPlayer->GetActorLocation());

                                UE_LOG(LogTemp, Warning, TEXT("CCTV 경보 전파: %s가 Suspicious -> Combat(전투) 상태로 최종 승격되었습니다!"), *Enemy->GetName());
                            }
                        }
                    }
                }
            }
        }
    }
}