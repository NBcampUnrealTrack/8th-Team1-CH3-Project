#include "AlertManager.h"
#include "EnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

AAlertManager::AAlertManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AAlertManager::BeginPlay()
{
    Super::BeginPlay();

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
void AAlertManager::ActivateAlert()
{
    if (bIsAlertActive) return;

    bIsAlertActive = true;

    UE_LOG(LogTemp, Warning, TEXT("AlertManager: 경보 활성화! 전체 적을 Combat으로 강제 전환합니다."));

    // 현재 레벨의 모든 적을 즉시 Combat으로 전환
    ForceAlertLevelToAllEnemies(EAlertLevel::Combat);

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
// 월드 내 모든 적 강제 전환
// ---------------------------------------------------------------
void AAlertManager::ForceAlertLevelToAllEnemies(EAlertLevel NewLevel)
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

        Enemy->OnAlertLevelChanged(NewLevel);

        // 블랙보드 TargetActor 없이 LastKnownLocation만 초기화
        // (플레이어 위치는 각 적이 직접 탐지해야 함)
        if (AAIController* AIC = Cast<AAIController>(Enemy->GetController()))
        {
            if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
            {
                BB->ClearValue(TEXT("TargetActor"));
                BB->ClearValue(TEXT("LastKnownLocation"));
            }
        }

        Count++;
    }

    UE_LOG(LogTemp, Warning, TEXT("AlertManager: %d명의 적을 AlertLevel %d로 강제 전환 완료"),
        Count, static_cast<int32>(NewLevel));
}