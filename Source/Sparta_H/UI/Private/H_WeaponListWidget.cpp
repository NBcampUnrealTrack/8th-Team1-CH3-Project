#include "UI/Public/H_WeaponListWidget.h"
#include "UI/Public/H_WeaponSlotWidget.h"
#include "Weapon/Public/WeaponDataAsset.h"
#include "Components/ScrollBox.h"

void UH_WeaponListWidget::RefreshWeaponList(const TArray<UWeaponDataAsset*>& Weapons)
{
	// 기존 목록 제거 후 새로운 무기 목록으로 슬롯 채우기
	if (!ScrollBox_List || !SlotWidgetClass) return;

	ScrollBox_List->ClearChildren();

	for (UWeaponDataAsset* Weapon : Weapons)
	{
		if (Weapon)
		{
			UH_WeaponSlotWidget* SlotWidget = CreateWidget<UH_WeaponSlotWidget>(this, SlotWidgetClass);
			if (SlotWidget)
			{
				SlotWidget->SetWeaponData(Weapon, TooltipWidgetClass);
				ScrollBox_List->AddChild(SlotWidget);
			}
		}
	}
}
