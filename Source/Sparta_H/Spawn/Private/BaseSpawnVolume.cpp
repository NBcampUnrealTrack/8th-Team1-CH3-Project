#include "Spawn/Public/BaseSpawnVolume.h"

#include "AlertManager.h"
#include "NavigationSystem.h"
#include "Components/BoxComponent.h"

ABaseSpawnVolume::ABaseSpawnVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));;
	SetRootComponent(SpawnArea);
	SpawnArea->SetBoxExtent(FVector(300.0f, 300.0f, 100.0f));
	SpawnArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABaseSpawnVolume::TriggerInitialSpawn()
{
	if (CurrentSpawnState != ESpawnState::Idle)
	{
		return;
	}
	SpawnEnemies(BaseSpawnCount, ESpawnState::Triggered);
}

void ABaseSpawnVolume::BeginPlay()
{
	Super::BeginPlay();

	if (AAlertManager* AlertMgr = AAlertManager::GetInstance(this))
	{
		AlertMgr->OnCombatEntered.AddDynamic(this, &ABaseSpawnVolume::HandleCombatEntered);
	}
}

void ABaseSpawnVolume::HandleCombatEntered(FVector CombatLocation)
{
	if (CurrentSpawnState != ESpawnState::Idle)
	{
		return;
	}

	const float DistSq = FVector::DistSquared(GetActorLocation(), CombatLocation);
	if (DistSq > FMath::Square(CombatTriggerRadius))
	{
		return;
	}

	SpawnEnemies(ExtraSpawnCount, ESpawnState::Completed);
}

bool ABaseSpawnVolume::GetRandomSpawnLocation(FVector& OutLocation) const
{
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys || !SpawnArea)
	{
		return false;
	}

	const FVector Origin = SpawnArea->GetComponentLocation();
	FNavLocation NavLoc;
	if (NavSys->GetRandomReachablePointInRadius(Origin, NavSearchRadius, NavLoc))
	{
		OutLocation = NavLoc.Location;
		return true;
	}
	return false;
}

void ABaseSpawnVolume::SpawnEnemies(int32 Count, ESpawnState NextStateOnSuccess)
{
	if (!EnemyClass || Count <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	int32 SpawnedCount = 0;
	for (int32 i = 0; i < Count; i++)
	{
		FVector SpawnLoc;
		if (!GetRandomSpawnLocation(SpawnLoc))
		{
			continue;
		}
		
		const FRotator SpawnRot = FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f);
		if (World->SpawnActor<APawn>(EnemyClass, SpawnLoc, SpawnRot, Params))
		{
			++SpawnedCount;
		}
	}

	if (SpawnedCount > 0)
	{
		CurrentSpawnState = NextStateOnSuccess;
	}
}
