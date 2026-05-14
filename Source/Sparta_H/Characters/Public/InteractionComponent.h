// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPARTA_H_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInteractionComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	
	void PerformInteraction(class UCameraComponent* Camera);
	
private:
	// 상호작용 가능 거리 (기존 300.0f 사용)
	UPROPERTY(EditAnywhere, Category = "Interaction")
	float TraceDistance = 200.0f;
};
