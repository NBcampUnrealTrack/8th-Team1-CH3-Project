#include "CCTV.h"
#include "GameFramework/CharacterMovementComponent.h"
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
#include "BlackboardKeys.h"

ACCTV::ACCTV()
{
    CurrentAlertLevel = EAlertLevel::CCTV;

    // 스켈레탈 메쉬 숨김 처리
    if (GetMesh())
    {
        GetMesh()->SetVisibility(false);
        GetMesh()->DestroyPhysicsState();
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // 중력 및 이동 비활성화 — CharacterMovement가 있으면 CCTV가 바닥으로 떨어짐
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->GravityScale = 0.f;
        MoveComp->SetMovementMode(MOVE_None);
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
    // Super::BeginPlay() 안에서 SetDefaultMovementMode()가 MOVE_None을 덮어쓰고
    // 위치/회전을 재조정하므로, 먼저 트랜스폼을 저장해 둔다
    const FVector SavedLocation = GetActorLocation();
    const FRotator SavedRotation = GetActorRotation();

    Super::BeginPlay();

    // 이동 모드 재비활성화 (Super가 Walking/Falling으로 되돌림)
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->DisableMovement();
        MoveComp->GravityScale = 0.f;
    }

    // 에디터에서 배치한 위치·회전 복원
    SetActorLocationAndRotation(SavedLocation, SavedRotation);

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

    // 기존 메시에 피직스 켜서 낙하
    if (CCTVMeshComp)
    {
        CCTVMeshComp->SetSimulatePhysics(true);
    }

    // 캡슐 콜리전 비활성화
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    UE_LOG(LogTemp, Warning, TEXT("CCTV [%s]: 파괴됨. 경보 전파 없음."), *GetName());

    // bIsDead = true, OnDeath 브로드캐스트
    Super::Die();
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
            BB->SetValueAsObject(BBKeys::TARGET_ACTOR, TargetPlayer);
            BB->SetValueAsVector(BBKeys::LAST_KNOWN_LOCATION, TargetPlayer->GetActorLocation());

            UE_LOG(LogTemp, Warning, TEXT("CCTV 경보: [%s] → Combat 전환"), *Enemy->GetName());
        }
    }
}