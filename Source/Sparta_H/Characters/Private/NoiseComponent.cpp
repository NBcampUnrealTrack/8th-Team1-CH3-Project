// Fill out your copyright notice in the Description page of Project Settings.


#include "NoiseComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values for this component's properties
UNoiseComponent::UNoiseComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UNoiseComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UNoiseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 이번 프레임에 수동 업데이트(AddNoise/SetNoiseToMax)가 있었다면 보간을 건너뜀
	if (bNoiseUpdatedThisFrame)
	{
		bNoiseUpdatedThisFrame = false;
		return;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	float TargetNoise = 0.0f;
	UCharacterMovementComponent* MovementComp = OwnerCharacter->GetCharacterMovement();
	
	if (MovementComp)
	{
		float VelocitySize = OwnerCharacter->GetVelocity().Size();
		float MaxSpeed = MovementComp->MaxWalkSpeed;
		if (MaxSpeed > 0.0f)
		{
			// 속도에 비례하여 최대 30.0f의 소음 발생 (기존 로직 유지)
			TargetNoise = (VelocitySize / MaxSpeed) * 30.0f;
		}
	}

	// 부드러운 변화를 위해 보간 사용
	float NewNoise = FMath::FInterpTo(CurrentNoise, TargetNoise, DeltaTime, NoiseInterpSpeed);
	
	if (!FMath::IsNearlyEqual(NewNoise, CurrentNoise))
	{
		CurrentNoise = NewNoise;
		OnNoiseChanged.Broadcast(CurrentNoise, MaxNoise);
	}
}

void UNoiseComponent::AddNoise(float Amount)
{
	CurrentNoise = FMath::Clamp(CurrentNoise + Amount, 0.0f, MaxNoise);
	// Modified: 수동 업데이트 플래그 설정
	bNoiseUpdatedThisFrame = true;
	OnNoiseChanged.Broadcast(CurrentNoise, MaxNoise);
}

void UNoiseComponent::SetNoiseToMax()
{
	CurrentNoise = MaxNoise;
	// Modified: 수동 업데이트 플래그 설정
	bNoiseUpdatedThisFrame = true;
	OnNoiseChanged.Broadcast(CurrentNoise, MaxNoise);
}

