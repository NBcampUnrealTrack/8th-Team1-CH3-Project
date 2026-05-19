#include "Hostage/public/HostageAIController.h"
#include "Hostage/public/HostageCharacter.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"

void AHostageAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 1. 비헤이비어 트리가 유효한지 먼저 철저히 검사
	if (!HostageBT)
	{
		UE_LOG(LogTemp, Error, TEXT("C++ Error : HostageBT Asset이 할당되지 않았습니다!"));
		return;
	}

	// 2. 비헤이비어 트리 가동 (이 함수 내부에서 블랙보드 컴포넌트가 자동으로 생성 및 바인딩됩니다)
	if (RunBehaviorTree(HostageBT))
	{
		// 3. 인질 캐릭터를 가져와 초기 상태 동기화
		if (AHostageCharacter* Hostage = Cast<AHostageCharacter>(InPawn))
		{
			if (UBlackboardComponent* BB = GetBlackboardComponent())
			{
				BB->SetValueAsEnum(TEXT("EHostageState"), static_cast<uint8>(Hostage->CurrentState));
                
				// 디버깅용 확인 로그
				UE_LOG(LogTemp, Log, TEXT("C++ : Hostage AI Possessed! Initial State Synced."));
			}
		}
	}
}