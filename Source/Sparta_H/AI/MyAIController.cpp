// Fill out your copyright notice in the Description page of Project Settings.


#include "MyAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Enemy/Public/EnemyCharacter.h"
#include "Enemy/Public/BlackboardKeys.h"

AMyAIController::AMyAIController()
{
	BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComp"));
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
}

void AMyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(InPawn);

	// 캐릭터에 지정된 BT 우선, 없으면 컨트롤러 기본 BT 사용
	UBehaviorTree* BTToRun = (EnemyChar && EnemyChar->GetEnemyBT()) ? EnemyChar->GetEnemyBT() : BTAsset;
	if (!BTToRun) return;

	if (RunBehaviorTree(BTToRun))
	{
		if (UBlackboardComponent* BB = GetBlackboardComponent())
		{
			if (EnemyChar)
			{
				BB->SetValueAsVector(BBKeys::LOCATION_A, EnemyChar->GetPatrolWorldLocationA());
				BB->SetValueAsVector(BBKeys::LOCATION_B, EnemyChar->GetPatrolWorldLocationB());
			}
		}
	}
}
