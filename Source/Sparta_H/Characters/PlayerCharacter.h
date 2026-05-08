// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerCharacter.generated.h"

struct FInputActionValue;

UENUM(BlueprintType)
enum class EPlayerMovementState : uint8
{
	Idle,
	Crouch,
	Walk,
	Sprint
};

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
	
	float MoveSpeed;
	float SprintSpeedMultiplier;
	float SprintSpeed;
	
	// 총기 착용 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	bool bIsEquipped = false;
	
	// 구르기 관련 , 일회성 동작이므로 몽타주 사용
	bool bIsRolling = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* DiveRollMontage;
	UFUNCTION()
	void OnRollMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	//기울이기 관련
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float LeanAmount =0.f;
	
	
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
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
	void StartLeanRight(const FInputActionValue& value);
	UFUNCTION()
	void StartLeanLeft(const FInputActionValue& value);
	UFUNCTION()
	void StopLean(const FInputActionValue& value);
	UFUNCTION()
	void Interaction(const FInputActionValue& value);
};
