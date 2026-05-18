#include "AlertManager.h"
#include "EnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "BlackboardKeys.h"

AAlertManager::AAlertManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AAlertManager::NotifyCombatEntered(FVector CombatLocation)
{
    OnCombatEntered.Broadcast(CombatLocation);
}

void AAlertManager::BeginPlay()
{
    Super::BeginPlay();
    BombAlertRadius = 3000.f;

    UE_LOG(LogTemp, Log, TEXT("AlertManager 초기화 완료 - 현재 상태: Off (모든 적 Idle 스폰)"));
}

// ---------------------------------------------------------------
// 싱글턴 접근
// ---------------------------------------------------------------
AAlertManager* AAlertManager::GetInstance(UObject* WorldContextObject)
{
    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (!World) return nullptr;

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AAlertManager::StaticClass(), FoundActors);

    if (FoundActors.Num() > 0)
    {
        return Cast<AAlertManager>(FoundActors[0]);
    }

    UE_LOG(LogTemp, Warning, TEXT("AlertManager: 레벨에 AlertManager 액터가 없습니다."));
    return nullptr;
}

// ---------------------------------------------------------------
// 경보 활성화 (폭탄 설치 시 MissionSystem에서 호출)
// ---------------------------------------------------------------
void AAlertManager::ActivateAlert(FVector BombLocation)
{
    if (bIsAlertActive) return;

    bIsAlertActive = true;

    UE_LOG(LogTemp, Warning, TEXT("AlertManager: 경보 활성화!"));

    ForceAlertLevelToNearbyEnemies(EAlertLevel::Lost, BombLocation, BombAlertRadius);

    // UI / 사운드 연동 브로드캐스트
    OnAlertActivated.Broadcast();
}

// ---------------------------------------------------------------
// 스폰 초기 AlertLevel 반환
// LevelSpawn에서 적 스폰 시 호출해서 초기 상태 설정
// ---------------------------------------------------------------
EAlertLevel AAlertManager::GetSpawnAlertLevel() const
{
    // Off → Idle(1) / On → Lost(4)
    return bIsAlertActive ? EAlertLevel::Lost : EAlertLevel::Idle;
}

// ---------------------------------------------------------------
// 폭탄 설치 지점 기준 반경 내 적만 강제 전환
// ---------------------------------------------------------------
void AAlertManager::ForceAlertLevelToNearbyEnemies(EAlertLevel NewLevel, FVector Origin, float Radius)
{
    UWorld* World = GetWorld();
    if (!World) return;

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AEnemyCharacter::StaticClass(), FoundActors);

    int32 Count = 0;

    for (AActor* Actor : FoundActors)
    {
        AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Actor);
        if (!Enemy || Enemy->IsDead()) continue;

        // 거리 체크 - 반경 밖이면 스킵
        float Distance = FVector::Dist(Origin, Enemy->GetActorLocation());
        if (Distance > Radius) continue;

        Enemy->OnAlertLevelChanged(NewLevel);

        if (AAIController* AIC = Cast<AAIController>(Enemy->GetController()))
        {
            if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
            {
                BB->ClearValue(TEXT("TargetActor"));
                BB->SetValueAsVector(TEXT("LastKnownLocation"), Origin);
            }
        }

        Count++;
    }

    UE_LOG(LogTemp, Warning, TEXT("AlertManager: 반경 %.0f 내 %d명의 적을 AlertLevel %d로 전환"),
        Radius, Count, static_cast<int32>(NewLevel));
}