#include "Spawn/Public//RespawnTrigger.h"

#include "PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Spawn/Public/BaseSpawnVolume.h"
#include "Kismet/GameplayStatics.h"

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

// 트리거 상태 초기화 및 현재 영역 내 플레이어 존재 시 즉시 실행
void ARespawnTrigger::ResetTrigger()
{
	bHasTriggered = false;

	if (TriggerBox)
	{
		// 1. 기본 오버랩 체크
		TArray<AActor*> OverlappingActors;
		TriggerBox->GetOverlappingActors(OverlappingActors, APlayerCharacter::StaticClass());
		
		bool bPlayerInside = (OverlappingActors.Num() > 0);

		// 텔레포트 직후 오버랩이 즉시 갱신되지 않았을 경우를 대비해 거리/범위로 직접 체크
		if (!bPlayerInside)
		{
			APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
			if (PlayerPawn && PlayerPawn->IsA(APlayerCharacter::StaticClass()))
			{
				const FVector PlayerLoc = PlayerPawn->GetActorLocation();
				const FVector BoxLoc = TriggerBox->GetComponentLocation();
				const FVector BoxExtent = TriggerBox->GetScaledBoxExtent();
				
				// AABB (Axis-Aligned Bounding Box) 충돌 체크 로직
				const bool bInX = FMath::Abs(PlayerLoc.X - BoxLoc.X) <= BoxExtent.X;
				const bool bInY = FMath::Abs(PlayerLoc.Y - BoxLoc.Y) <= BoxExtent.Y;
				const bool bInZ = FMath::Abs(PlayerLoc.Z - BoxLoc.Z) <= BoxExtent.Z;

				if (bInX && bInY && bInZ)
				{
					bPlayerInside = true;
				}
			}
		}

		if (bPlayerInside)
		{
			UE_LOG(LogTemp, Log, TEXT("RespawnTrigger: 플레이어가 영역 내에 있음이 감지됨 (직접 체크). 즉시 스폰 실행."));
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
