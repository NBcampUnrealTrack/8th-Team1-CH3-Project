// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

/**
 * 
 */

class UInputMappingContext;
class UInputAction;

UCLASS()
class SPARTA_H_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMyPlayerController();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputMappingContext* InputMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* MoveAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* JumpAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* LookAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* RunAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* HideAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* RollAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* LeanRightAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* LeanLeftAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* Interaction;

	// 무기 시스템 입력. 캐릭터에서 이 멤버를 참조해 BindAction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Weapon")
	UInputAction* FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Weapon")
	UInputAction* ReloadAction;

	// 1~4 키로 슬롯 직접 선택 (Axis1D, Scalar Modifier 1~4)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Weapon")
	UInputAction* EquipSlotAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Weapon")
	UInputAction* EquipNextWeaponAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Weapon")
	UInputAction* EquipPreviousWeaponAction;

protected:
	virtual void BeginPlay() override;
};
