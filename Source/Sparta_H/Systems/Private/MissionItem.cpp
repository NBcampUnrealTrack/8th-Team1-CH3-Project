#include "Systems/Public/MissionItem.h"
#include "Characters/Public/PlayerCharacter.h"
#include "Components/StaticMeshComponent.h"

AMissionItem::AMissionItem()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
}

void AMissionItem::Interact_Implementation(APlayerCharacter* Interactor)
{
	if (bIsUsed || !Interactor) return;

	// 현재 활성화된 목표가 이 아이템의 ID와 일치하는지 확인
	if (!TargetGoalID.IsNone() && !Interactor->IsCurrentObjective(TargetGoalID))
	{
		UE_LOG(LogTemp, Log, TEXT("Mission Item: Not the current objective."));
		return;
	}

	// 플레이어의 미션 완료 처리
	Interactor->CompleteCurrentObjective();

	bIsUsed = true;
	UE_LOG(LogTemp, Warning, TEXT("Mission Item Interacted: %s"), *TargetGoalID.ToString());

	if (bDestroyOnInteract)
	{
		Destroy();
	}
}

bool AMissionItem::CanInteract_Implementation(APlayerCharacter* Interactor) const
{
	if (bIsUsed || !Interactor) return false;

	// 현재 활성화된 목표인 경우에만 상호작용 가능
	if (!TargetGoalID.IsNone())
	{
		return Interactor->IsCurrentObjective(TargetGoalID);
	}

	return true;
}

FString AMissionItem::GetInteractionText_Implementation() const
{
	return InteractionText;
}
