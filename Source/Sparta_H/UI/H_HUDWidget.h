#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Weapon/WeaponTypes.h"
#include "H_HUDWidget.generated.h"

class ASparta_HCharacter;

/**
 * WBP_MainHUD의 베이스가 되는 C++ 클래스입니다.
 * 플레이어 캐릭터로부터 데이터를 가져와 위젯에 노출하는 브릿지 역할을 합니다.
 */
UCLASS()
class SPARTA_H_API UH_HUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 위젯에서 캐릭터 정보에 접근하기 위한 헬퍼 함수
	UFUNCTION(BlueprintCallable, Category = "HUD")
	ASparta_HCharacter* GetOwningCharacter() const;

	/** 체력 및 스태미너 관련 **/
	UFUNCTION(BlueprintCallable, Category = "HUD|Stats")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintCallable, Category = "HUD|Stats")
	FText GetHealthText() const;

	UFUNCTION(BlueprintCallable, Category = "HUD|Stats")
	float GetStaminaPercent() const;

	UFUNCTION(BlueprintCallable, Category = "HUD|Stats")
	FText GetStaminaText() const;

	/** 무기 및 탄약 관련 **/
	UFUNCTION(BlueprintCallable, Category = "HUD|Weapon")
	FText GetWeaponName() const;

	UFUNCTION(BlueprintCallable, Category = "HUD|Weapon")
	FText GetAmmoText() const;


	UFUNCTION(BlueprintCallable, Category = "HUD|Weapon")
	ECrosshairState GetCrosshairState() const;

	/** 목표 관련 **/
	UFUNCTION(BlueprintCallable, Category = "HUD|Objective")
	FText GetObjectiveText() const;
};
