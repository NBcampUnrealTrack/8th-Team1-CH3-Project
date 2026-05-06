#include "H_HUDWidget.h"
#include "../Sparta_HCharacter.h"
#include "../Weapon/WeaponBase.h"
#include "../Weapon/AmmoComponent.h"
#include "../Weapon/WeaponDataAsset.h"

ASparta_HCharacter* UH_HUDWidget::GetOwningCharacter() const
{
	return Cast<ASparta_HCharacter>(GetOwningPlayerPawn());
}

float UH_HUDWidget::GetHealthPercent() const
{
	if (const ASparta_HCharacter* Character = GetOwningCharacter())
	{
		return (Character->MaxHealth > 0.0f) ? (Character->CurrentHealth / Character->MaxHealth) : 0.0f;
	}
	return 0.0f;
}

FText UH_HUDWidget::GetHealthText() const
{
	if (const ASparta_HCharacter* Character = GetOwningCharacter())
	{
		return FText::Format(FText::FromString(TEXT("{0} / {1}")), 
			FText::AsNumber(FMath::FloorToInt(Character->CurrentHealth)), 
			FText::AsNumber(FMath::FloorToInt(Character->MaxHealth)));
	}
	return FText::GetEmpty();
}

FText UH_HUDWidget::GetWeaponName() const
{
	if (const ASparta_HCharacter* Character = GetOwningCharacter())
	{
		if (const UWeaponDataAsset* Data = Character->GetCurrentWeaponData())
		{
			return FText::FromString(TEXT("웨폰 이름이 없어요")); // Data->WeaponName;
		}
	}
	return FText::FromString(TEXT("장착한 무기 없음"));
}

FText UH_HUDWidget::GetAmmoText() const
{
	if (const ASparta_HCharacter* Character = GetOwningCharacter())
	{
		if (const AWeaponBase* Weapon = Character->GetCurrentWeapon())
		{
			if (const UAmmoComponent* Ammo = Weapon->GetAmmoComponent())
			{
				return FText::Format(FText::FromString(TEXT("{0} / {1}")), 
					FText::AsNumber(Ammo->GetCurrentAmmoCount()), 
					FText::AsNumber(Ammo->GetMaxAmmoCount()));
			}
		}
	}
	return FText::GetEmpty();
}

FText UH_HUDWidget::GetObjectiveText() const
{
	if (const ASparta_HCharacter* Character = GetOwningCharacter())
	{
		return FText::FromString(Character->CurrentObjective);
	}
	return FText::GetEmpty();
}
