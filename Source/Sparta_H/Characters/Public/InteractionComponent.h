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
	
protected:
	// 상호작용 가능 거리 (기존 300.0f 사용)
	UPROPERTY(EditAnywhere, Category = "Interaction")
	float TraceDistance = 200.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractionCooldown = 1.f;
	
	bool bIsCooldown = false;

	// 타이머 관리를 위한 핸들
	FTimerHandle CooldownTimerHandle;

	// 쿨타임이 끝났을 때 호출될 함수
	void ResetInteractionCooldown();
};
