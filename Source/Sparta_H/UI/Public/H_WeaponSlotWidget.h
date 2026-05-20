#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "H_WeaponSlotWidget.generated.h"

class UWeaponDataAsset;
class UImage;
class UTextBlock;
class UH_WeaponDetailsWidget;

/**
 * 인벤토리 목록의 개별 무기 슬롯 위젯
 */
UCLASS()
class SPARTA_H_API UH_WeaponSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 슬롯에 무기 데이터를 설정
	UFUNCTION(BlueprintCallable, Category = "UI|Weapon")
	void SetWeaponData(UWeaponDataAsset* InData, TSubclassOf<UH_WeaponDetailsWidget> InTooltipClass);

protected:
	// 마우스 오버 시 툴팁을 생성하여 반환
	// 마우스 오버 시 호출될 툴팁 생성 함수
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidget))
	UImage* Image_Icon;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_Name;

	UPROPERTY()
	UWeaponDataAsset* WeaponData;

	UPROPERTY()
	TSubclassOf<UH_WeaponDetailsWidget> TooltipClass;
};
