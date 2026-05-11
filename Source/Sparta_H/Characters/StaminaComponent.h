// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StaminaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaChanged, float, CurrentStamina, float, MaxStamina);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPARTA_H_API UStaminaComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UStaminaComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	void SetSprinting(bool bNewSprinting) { bIsSprinting = bNewSprinting;}
	
	// 스태미나가 충분한지 확인
	bool CanSprint() const { return CurrentStamina > 0.f; }

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnStaminaChanged OnStaminaChanged;

private:
	UPROPERTY(EditAnywhere, Category = "Stamina")
	float MaxStamina = 100.f;

	UPROPERTY(VisibleAnywhere, Category = "Stamina")
	float CurrentStamina;

	UPROPERTY(EditAnywhere, Category = "Stamina")
	float ConsumptionRate = 20.f; // 초당 소모량

	UPROPERTY(EditAnywhere, Category = "Stamina")
	float RecoveryRate = 15.f;    // 초당 회복량

	bool bIsSprinting = false;
};
