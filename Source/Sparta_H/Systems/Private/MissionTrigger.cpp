#include "MissionTrigger.h"
#include "Components/BoxComponent.h"
#include "PlayerCharacter.h"
#include "Kismet/GameplayStatics.h"

AMissionTrigger::AMissionTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;

	// 충돌 설정 강화
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->SetGenerateOverlapEvents(true);
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
	if (bIsUsed)
	{
		return;
	}

	if (APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor))
	{
		UE_LOG(LogTemp, Log, TEXT("MissionTrigger: Player overlapped with trigger. TargetGoalID: %s"),
		       *TargetGoalID.ToString());

		// 플레이어에게 미션 데이터가 있는지 확인
		if (!Player->CurrentMissionData)
		{
			UE_LOG(LogTemp, Error, TEXT("MissionTrigger: Player has no CurrentMissionData assigned!"));
			return;
		}

		// 이 트리거가 특정 목표를 담당하는 경우, 현재 활성화된 목표인지 확인
		if (!TargetGoalID.IsNone())
		{
			if (!Player->IsCurrentObjective(TargetGoalID))
			{
				FName CurrentGoalID = Player->GetCurrentObjectiveID();
				UE_LOG(LogTemp, Warning, TEXT("MissionTrigger: GoalID mismatch. Expected: %s, Current: %s"),
				       *TargetGoalID.ToString(), *CurrentGoalID.ToString());
				return;
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("MissionTrigger: Successfully completing objective: %s"),
		       *TargetGoalID.ToString());
		Player->CompleteCurrentObjective();

		// if (CompletionSound)
		// {
		// 	UGameplayStatics::PlaySoundAtLocation(this, CompletionSound, GetActorLocation());
		// }

		// bIsUsed = true;
	}
}
