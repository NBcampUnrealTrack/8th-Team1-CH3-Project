#include "Spawn/Public//RespawnTrigger.h"

#include "PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Spawn/Public/BaseSpawnVolume.h"

ARespawnTrigger::ARespawnTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(200.0f, 200.0f, 100.0f));
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerBox->SetGenerateOverlapEvents(true);
}

void ARespawnTrigger::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(
			this, &ARespawnTrigger::HandleOnComponentBeginOverlap);
	}
}

void ARespawnTrigger::HandleOnComponentBeginOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	if (bHasTriggered)
	{
		return;
	}
	// 플레이어만 트리거
	if (!OtherActor || !OtherActor->IsA(APlayerCharacter::StaticClass()))
	{
		return;
	}

	for (ABaseSpawnVolume* Volume : TargetSpawnVolumes)
	{
		if (Volume)
		{
			Volume->TriggerInitialSpawn();
		}
	}

	bHasTriggered = true;
}
