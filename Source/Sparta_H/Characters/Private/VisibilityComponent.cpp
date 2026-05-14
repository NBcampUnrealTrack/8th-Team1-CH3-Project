// Fill out your copyright notice in the Description page of Project Settings.


#include "VisibilityComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values for this component's properties
UVisibilityComponent::UVisibilityComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UVisibilityComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

float UVisibilityComponent::GetVisibilityScore() const
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!Owner) return 1.0f;

	float FinalScore = 1.0f;
	
	if (Owner->GetCharacterMovement()->IsCrouching())
	{
		FinalScore *= CrouchingMultiplier;
	}
	else
	{
		FinalScore *= StandingMultiplier;
	}
	
	FinalScore *= CalculateLightFactor();

	return FMath::Clamp(FinalScore, 0.0f, 1.0f);
}

float UVisibilityComponent::CalculateLightFactor()
{
	// 조명이나 엄폐물에 따른 요소를 계산하는 로직 여기에 추가
	return 1.0f; 
}
