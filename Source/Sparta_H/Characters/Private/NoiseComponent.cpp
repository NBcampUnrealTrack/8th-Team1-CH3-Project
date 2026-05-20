// Fill out your copyright notice in the Description page of Project Settings.


#include "NoiseComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Perception/AISense_Hearing.h"

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

	FootstepReportAccum += DeltaTime;
	if (FootstepReportAccum >= FootstepReportInterval)
	{
		FootstepReportAccum = 0.f;
		if (CurrentNoise >= FootstepNoiseThreshold)
		{
			ReportFootstepNoise();
		}
	}
}

void UNoiseComponent::ReportFootstepNoise()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;

	// 발소리 최대치는 30.0f (속도 기반)이므로 MaxNoise(100)가 아닌 30 기준으로 정규화
	static constexpr float MaxFootstepNoise = 30.f;
	const float LoudnessRatio = FMath::Clamp(CurrentNoise / MaxFootstepNoise, 0.f, 1.f);
	const float EffectiveRange = MaxFootstepHearingRange * LoudnessRatio;
	UE_LOG(LogTemp, Log, TEXT("[Footstep] 발소리 AI 보고 | Noise=%.1f | Range=%.0f"), CurrentNoise, EffectiveRange);
	UAISense_Hearing::ReportNoiseEvent(
		GetWorld(),
		OwnerPawn->GetActorLocation(),
		LoudnessRatio,
		OwnerPawn,
		EffectiveRange,
		FName("Footstep")
	);
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

