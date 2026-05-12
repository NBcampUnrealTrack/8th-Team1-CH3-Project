// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MyAIController.generated.h"

/**
 * 
 */
UCLASS()
class SPARTA_H_API AMyAIController : public AAIController
{
	GENERATED_BODY()
public:
	AMyAIController();
    
	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	UBehaviorTree* BTAsset;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	UBlackboardData* BBAsset;

	// UseBlackboard 함수가 결과를 담을 컴포넌트 변수가 필요합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UBlackboardComponent* BlackboardComp;
};
