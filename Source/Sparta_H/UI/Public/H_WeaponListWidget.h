#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "H_WeaponListWidget.generated.h"

class UScrollBox;
class UH_WeaponSlotWidget;
class UWeaponDataAsset;
class UH_WeaponDetailsWidget;

/**
 * Tab 키를 눌렀을 때 표시될 전체 무기 목록 위젯
 */
UCLASS()
class SPARTA_H_API UH_WeaponListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 무기 목록을 초기화하고 생성합니다.
	// 캐릭터의 소지 무기 목록을 기반으로 슬롯 생성
	UFUNCTION(BlueprintCallable, Category = "UI|Weapon")
	void RefreshWeaponList(const TArray<UWeaponDataAsset*>& Weapons);

protected:
	UPROPERTY(meta = (BindWidget))
	UScrollBox* ScrollBox_List;

	// 개별 슬롯으로 사용할 위젯 클래스 (에디터에서 지정)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Weapon")
	TSubclassOf<UH_WeaponSlotWidget> SlotWidgetClass;

	// 툴팁으로 사용할 위젯 클래스 (에디터에서 지정)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Weapon")
	TSubclassOf<UH_WeaponDetailsWidget> TooltipWidgetClass;
};
