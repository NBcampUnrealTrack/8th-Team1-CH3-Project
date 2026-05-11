#include "MissionTrigger.h"
#include "Components/BoxComponent.h"
#include "PlayerCharacter.h"
#include "Kismet/GameplayStatics.h"

AMissionTrigger::AMissionTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
}

void AMissionTrigger::BeginPlay()
{
	Super::BeginPlay();
	
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AMissionTrigger::OnOverlapBegin);
}

void AMissionTrigger::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
                                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
                                     bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsUsed) return;

	if (APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor))
	{
		// 이 트리거가 특정 목표를 담당하는 경우, 현재 활성화된 목표인지 확인
		if (!TargetGoalID.IsNone() && !Player->IsCurrentObjective(TargetGoalID))
		{
			// 현재 목표가 아니면 무시 (순차 진행 보장)
			return;
		}

		Player->CompleteCurrentObjective();
		
		if (CompletionSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, CompletionSound, GetActorLocation());
		}

		bIsUsed = true;
		UE_LOG(LogTemp, Warning, TEXT("Mission Objective Triggered: %s"), *TargetGoalID.ToString());
	}
}
