#include "H_WeaponWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "PlayerCharacter.h"
#include "WeaponBase.h"
#include "AmmoComponent.h"
#include "WeaponDataAsset.h"

void UH_WeaponWidget::UpdateFromCharacter(APlayerCharacter* Character)
{
	if (!Character)
	{
		return;
	}

	FText Name;
	UTexture2D* Icon = nullptr;
	int32 Current = 0;
	int32 Max = 0;

	// 데이터 에셋에서 무기 정보(이름, 아이콘) 가져오기
	if (const UWeaponDataAsset* Data = Character->GetCurrentWeaponData())
	{
		Name = Data->WeaponName;

		// SoftObjectPtr 처리: 이미 로드되어 있으면 가져오고, 아니면 동기 로드
		if (Data->WeaponIcon.IsPending())
		{
			Icon = Data->WeaponIcon.LoadSynchronous();
		}
		else
		{
			Icon = Data->WeaponIcon.Get();
		}
	}

	// 무기 액터 및 탄약 컴포넌트에서 현재 탄약 정보 가져오기
	if (const AWeaponBase* Weapon = Character->GetCurrentWeapon())
	{
		if (const UAmmoComponent* Ammo = Weapon->GetAmmoComponent())
		{
			Current = Ammo->GetCurrentAmmoCount();
			Max = Ammo->GetMaxAmmoCount();
		}
	}

	UpdateWeaponInfo(Name, Icon, Current, Max);
}

void UH_WeaponWidget::UpdateWeaponInfo(const FText& WeaponName, UTexture2D* WeaponIcon, int32 CurrentAmmo,
                                       int32 MaxAmmo)
{
	// Modified: UI 위젯에 데이터 바인딩
	if (WeaponNameText)
	{
		WeaponNameText->SetText(WeaponName);
	}

	if (WeaponIconImage)
	{
		if (WeaponIcon)
		{
			WeaponIconImage->SetBrushFromTexture(WeaponIcon);
			WeaponIconImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			WeaponIconImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (AmmoText)
	{
		// 형식: "현재 / 최대"
		FString AmmoString = FString::Printf(TEXT("%d / %d"), CurrentAmmo, MaxAmmo);
		AmmoText->SetText(FText::FromString(AmmoString));
	}
}
