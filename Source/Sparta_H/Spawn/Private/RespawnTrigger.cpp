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

// Modified: 트리거 상태 초기화 및 현재 영역 내 플레이어 존재 시 즉시 실행
void ARespawnTrigger::ResetTrigger()
{
	bHasTriggered = false;

	// Modified: 체크포인트가 트리거 영역과 겹칠 경우, 리스폰 즉시 적을 스폰하도록 체크
	if (TriggerBox)
	{
		TArray<AActor*> OverlappingActors;
		TriggerBox->GetOverlappingActors(OverlappingActors, APlayerCharacter::StaticClass());
		
		if (OverlappingActors.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("RespawnTrigger: 플레이어가 이미 영역 내에 있음. 즉시 스폰 실행."));
			ExecuteSpawn();
		}
	}
}

// Modified: 실제 스폰 실행 로직 분리
void ARespawnTrigger::ExecuteSpawn()
{
	if (bHasTriggered)
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

	// Modified: 분리된 실행 함수 호출
	ExecuteSpawn();
}
