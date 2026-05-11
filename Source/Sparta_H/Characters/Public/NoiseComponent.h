// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NoiseComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNoiseChanged, float, CurrentNoise, float, MaxNoise);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPARTA_H_API UNoiseComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UNoiseComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	float GetCurrentNoise() const { return CurrentNoise; }
	float GetMaxNoise() const { return MaxNoise; }

	UFUNCTION(BlueprintCallable, Category = "Noise")
	void AddNoise(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Noise")
	void SetNoiseToMax();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnNoiseChanged OnNoiseChanged;

private:
	UPROPERTY(EditAnywhere, Category = "Player|Stats")
	float CurrentNoise = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Player|Stats")
	float MaxNoise = 100.0f;
	
	UPROPERTY(EditAnywhere, Category = "Player|Stats")
	float NoiseInterpSpeed = 10.0f;
};
