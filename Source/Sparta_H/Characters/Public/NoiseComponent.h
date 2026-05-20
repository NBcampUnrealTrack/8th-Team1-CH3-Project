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
	void ReportFootstepNoise();

	bool bNoiseUpdatedThisFrame = false;
	float FootstepReportAccum = 0.f;

	UPROPERTY(EditAnywhere, Category = "Player|Stats")
	float CurrentNoise = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Player|Stats")
	float MaxNoise = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Player|Stats")
	float NoiseInterpSpeed = 10.0f;

	// 발소리 AI 감지: 최대 소음일 때 탐지 반경 (cm)
	UPROPERTY(EditAnywhere, Category = "Player|Noise|AI")
	float MaxFootstepHearingRange = 800.f;

	// 발소리를 AI에 보고할 최소 소음 수치 (이 이하면 무시)
	UPROPERTY(EditAnywhere, Category = "Player|Noise|AI")
	float FootstepNoiseThreshold = 5.f;

	// AI에 발소리를 보고하는 주기 (초)
	UPROPERTY(EditAnywhere, Category = "Player|Noise|AI")
	float FootstepReportInterval = 0.25f;
};
