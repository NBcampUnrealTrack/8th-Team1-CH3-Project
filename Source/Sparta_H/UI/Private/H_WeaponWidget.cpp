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

	AWeaponBase* MainWeapon = Character->GetMainWeapon();
	if (!MainWeapon)
	{
		SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	SetVisibility(ESlateVisibility::Visible);

	FText Name;
	UTexture2D* Icon = nullptr;
	int32 Current = 0;
	int32 Max = 0;

	// 메인 무기의 데이터 에셋에서 정보 가져오기
	if (const UWeaponDataAsset* Data = MainWeapon->GetWeaponData())
	{
		Name = Data->WeaponName;
		Icon = Data->WeaponIcon.LoadSynchronous();
	}

	// 메인 무기의 탄약 정보 가져오기
	if (const UAmmoComponent* Ammo = MainWeapon->GetAmmoComponent())
	{
		Current = Ammo->GetCurrentAmmoCount();
		Max = Ammo->GetMaxAmmoCount();
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
