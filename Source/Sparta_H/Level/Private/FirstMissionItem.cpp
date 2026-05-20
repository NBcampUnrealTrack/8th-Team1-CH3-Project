#include "Level/Public/FirstMissionItem.h"
#include "PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MissionDataAsset.h"

AFirstMissionItem::AFirstMissionItem()
{
	// 아이템의 물리 속성과 센서 크기 조절
	if (MainMesh)
	{
		MainMesh->SetCollisionObjectType(ECC_WorldDynamic);
		MainMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MainMesh->SetCollisionResponseToAllChannels(ECR_Block);
	}

	if (SensorBox)
	{
		SensorBox->SetBoxExtent(FVector(200.f, 200.f, 200.f));
		SensorBox->SetCollisionProfileName(TEXT("Trigger"));
	}
}

void AFirstMissionItem::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	{
		if (Player->CurrentMissionData)
		{
			for (FMissionGoal& Goal : Player->CurrentMissionData->MissionGoals)
			{
				if (Goal.GoalID == TargetGoalID)
				{
					Goal.TargetLocation = GetActorLocation();
					break;
				}
			}
		}
	}
}

void AFirstMissionItem::Interact_Implementation(APlayerCharacter* Interactor)
{
	if (bIsUsed || !Interactor) return;
	
	if (bUnlockRifleOnInteract)
	{
		Interactor->bHasRifle = true;
	}

	Interactor->CompleteCurrentObjective();
	bIsUsed = true;
	Destroy();
}

bool AFirstMissionItem::CanInteract_Implementation(APlayerCharacter* Interactor) const
{
	return bIsPlayerNearby && !bIsUsed;
}

FString AFirstMissionItem::GetInteractionText_Implementation() const
{
	return TEXT("아이템 줍기 (F)");
}