// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VisibilityComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPARTA_H_API UVisibilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVisibilityComponent();
	
	UFUNCTION(BlueprintCallable, Category = "Visibility")
	float GetVisibilityScore() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
private:
	// 자세에 따른 가시성 보정값
	UPROPERTY(EditAnywhere, Category = "Settings")
	float StandingMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Settings")
	float CrouchingMultiplier = 0.5f; // 웅크리면 발각 확률 절반

	// 조명 상태에 따른 보정 (간단한 구현용)
	static float CalculateLightFactor();

		
};
