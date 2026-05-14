// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractionComponent.h"
#include "MissionInteractableInterface.h"
#include "Camera//CameraComponent.h"
#include "Engine/World.h"
#include "PlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"

// Sets default values for this component's properties
UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts
void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UInteractionComponent::PerformInteraction(UCameraComponent* Camera)
{
	if (!Camera || !GetWorld()) return;
	
	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + (Camera->GetForwardVector() * TraceDistance);
	
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner()); // 컴포넌트 소유자(플레이어) 무시

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);

	// --- 디버그 라인 출력 ---
	// 액터 시 초록색, 맞으면 노란색 미적중 시 빨간색으로 표시
	FColor LineColor = FColor::Red;
	
	if (bHit)
	{
		LineColor = FColor::Yellow; //
		
		if (AActor* HitActor = HitResult.GetActor())
		{
			APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwner());
			
			if (HitActor->GetClass()->ImplementsInterface(UMissionInteractableInterface::StaticClass()))
			{
				LineColor = FColor::Green;
				
				if (IMissionInteractableInterface::Execute_CanInteract(HitActor, Player))
				{
					IMissionInteractableInterface::Execute_Interact(HitActor, Player);
				}
			}
		}
	}
	
	DrawDebugLine(GetWorld(), Start, End, LineColor, false, 2.0f, 0, 1.0f);
}
