// Fill out your copyright notice in the Description page of Project Settings.


#include "StaminaComponent.h"

// Sets default values for this component's properties
UStaminaComponent::UStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	CurrentStamina = MaxStamina;
}


// Called when the game starts
void UStaminaComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	float PreviousStamina = CurrentStamina;

	if (bIsSprinting)
	{
		// 달리기 중일 때 소모
		CurrentStamina = FMath::Clamp(CurrentStamina - (ConsumptionRate * DeltaTime), 0.f, MaxStamina);
	}
	else
	{
		// 아닐 때 회복
		CurrentStamina = FMath::Clamp(CurrentStamina + (RecoveryRate * DeltaTime), 0.f, MaxStamina);
	}

	// 수치가 변했을 때만 델리게이트 호출
	if (!FMath::IsNearlyEqual(PreviousStamina, CurrentStamina))
	{
		OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
	}
}

