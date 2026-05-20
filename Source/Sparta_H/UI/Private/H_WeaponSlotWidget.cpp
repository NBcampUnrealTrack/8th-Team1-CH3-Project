#include "UI/Public/H_WeaponSlotWidget.h"
#include "Weapon/Public/WeaponDataAsset.h"
#include "UI/Public/H_WeaponDetailsWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

void UH_WeaponSlotWidget::SetWeaponData(UWeaponDataAsset* InData, TSubclassOf<UH_WeaponDetailsWidget> InTooltipClass)
{
	// 슬롯에 무기 아이콘과 이름 설정
	WeaponData = InData;
	TooltipClass = InTooltipClass;

	if (WeaponData)
	{
		if (Image_Icon)
		{
			// TSoftObjectPtr 로드 처리
			UTexture2D* Icon = WeaponData->WeaponIcon.LoadSynchronous();
			Image_Icon->SetBrushFromTexture(Icon);
		}
		if (Text_Name)
		{
			Text_Name->SetText(WeaponData->WeaponName);
		}
	}
}

void UH_WeaponSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	// 마우스 진입 시 툴팁 위젯 생성 및 데이터 할당
	if (WeaponData && TooltipClass)
	{
		UH_WeaponDetailsWidget* TooltipWidget = CreateWidget<UH_WeaponDetailsWidget>(this, TooltipClass);
		if (TooltipWidget)
		{
			TooltipWidget->UpdateWeaponDetails(WeaponData);
			SetToolTip(TooltipWidget);
		}
	}
}
