#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "HostageAIController.generated.h"

UCLASS()
class SPARTA_H_API AHostageAIController : public AAIController
{
	GENERATED_BODY()

protected:
	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	class UBehaviorTree* HostageBT;
};