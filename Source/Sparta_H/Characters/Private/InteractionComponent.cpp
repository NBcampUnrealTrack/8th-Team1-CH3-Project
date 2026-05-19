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
	// 1. 필수 컴포넌트 체크 및 쿨타임 중인지 검사
	if (!Camera || !GetWorld() || bIsCooldown) return;
    
	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + (Camera->GetForwardVector() * TraceDistance);
    
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);

	FColor LineColor = FColor::Red;
    
	if (bHit)
	{
		LineColor = FColor::Yellow;
        
		if (AActor* HitActor = HitResult.GetActor())
		{
			APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwner());
            
			if (HitActor->GetClass()->ImplementsInterface(UMissionInteractableInterface::StaticClass()))
			{
				LineColor = FColor::Green;
                
				if (IMissionInteractableInterface::Execute_CanInteract(HitActor, Player))
				{
					// 실제 인터랙션 실행
					IMissionInteractableInterface::Execute_Interact(HitActor, Player);
                    
					// 2. 인터랙션 성공 시 쿨타임 가동
					bIsCooldown = true;
                    
					// InteractionCooldown만큼의 시간이 흐른 뒤 ResetInteractionCooldown 함수를 호출함
					GetWorld()->GetTimerManager().SetTimer(
						CooldownTimerHandle, 
						this, 
						&UInteractionComponent::ResetInteractionCooldown, 
						InteractionCooldown, 
						false
					);
				}
			}
		}
	}
    
	DrawDebugLine(GetWorld(), Start, End, LineColor, false, 2.0f, 0, 1.0f);
}

void UInteractionComponent::ResetInteractionCooldown()
{
	bIsCooldown = false;
    
	// 타이머 핸들 깔끔하게 정리 (선택 사항이지만 권장)
	GetWorld()->GetTimerManager().ClearTimer(CooldownTimerHandle);
}
