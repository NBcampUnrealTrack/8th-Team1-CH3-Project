#include "H_WeaponWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "../Characters/PlayerCharacter.h"
#include "../Weapon/WeaponBase.h"
#include "../Weapon/AmmoComponent.h"
#include "../Weapon/WeaponDataAsset.h"

void UH_WeaponWidget::UpdateFromCharacter(APlayerCharacter* Character)
{
	if (!Character) return;

	FString Name = TEXT("No Weapon");
	UTexture2D* Icon = nullptr;
	int32 Current = 0;
	int32 Max = 0;

	if (const UWeaponDataAsset* Data = Character->GetCurrentWeaponData())
	{
		// Name = Data->WeaponName;
		// Icon = Data->WeaponIcon;
	}

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

void UH_WeaponWidget::UpdateWeaponInfo(const FString& WeaponName, UTexture2D* WeaponIcon, int32 CurrentAmmo, int32 MaxAmmo)
{
	
}
