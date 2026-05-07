// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerCharacter.generated.h"

struct FInputActionValue;

UCLASS()
class SPARTA_H_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerCharacter();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class USpringArmComponent* SpringArm;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class UCameraComponent* Camera;
	

protected:
	float MoveSpeed;
	float SprintSpeedMultiplier;
	float SprintSpeed;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	bool IsEquipped = false;
	
	
public:	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	UFUNCTION()
	void Move(const FInputActionValue& value);
	UFUNCTION()
	void Look(const FInputActionValue& value);
	UFUNCTION()
	void StartJump(const FInputActionValue& value);
	UFUNCTION()
	void StopJump(const FInputActionValue& value);
	UFUNCTION()
	void StartRun(const FInputActionValue& value);
	UFUNCTION()
	void StopRun(const FInputActionValue& value);
	UFUNCTION()
	void StartHide(const FInputActionValue& value);
	UFUNCTION()
	void StopHide(const FInputActionValue& value);
	UFUNCTION()
	void Roll(const FInputActionValue& value);
	UFUNCTION()
	void StartLean(const FInputActionValue& value);
	UFUNCTION()
	void StopLean(const FInputActionValue& value);
	UFUNCTION()
	void Interaction(const FInputActionValue& value);
};
