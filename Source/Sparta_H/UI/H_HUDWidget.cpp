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
		return FText::Format(FText::FromString(TEXT("HP :       {0} / {1}")),
		                     FText::AsNumber(FMath::FloorToInt(Character->CurrentHealth)),
		                     FText::AsNumber(FMath::FloorToInt(Character->MaxHealth)));
	}
	return FText::GetEmpty();
}

float UH_HUDWidget::GetStaminaPercent() const
{
	if (const ASparta_HCharacter* Character = GetOwningCharacter())
	{
		return 0; // (Character->MaxStamina > 0.0f) ? (Character->CurrentStamina / Character->MaxStamina) : 0.0f;
	}
	return 0.0f;
}

FText UH_HUDWidget::GetStaminaText() const
{
	if (const ASparta_HCharacter* Character = GetOwningCharacter())
	{
		return FText::Format(FText::FromString(TEXT("Stamina :  {0} / {1}")), 0, 0);
		// 0,0을 스태미나로 수정 
		// FText::AsNumber(FMath::FloorToInt(Character->CurrentStamina)), 
		// FText::AsNumber(FMath::FloorToInt(Character->MaxStamina)));
	}
	return FText::GetEmpty();
}

FText UH_HUDWidget::GetWeaponName() const
{
	if (const ASparta_HCharacter* Character = GetOwningCharacter())
	{
		if (const UWeaponDataAsset* Data = Character->GetCurrentWeaponData())
		{
			return FText::FromString(TEXT("무기 이름")); // Data->WeaponName;
		}
	}
	return FText::FromString(TEXT("No Weapon"));
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

ECrosshairState UH_HUDWidget::GetCrosshairState() const
{
	if (const ASparta_HCharacter* Character = GetOwningCharacter())
	{
		return Character->CurrentCrosshairState;
	}
	return ECrosshairState::Default;
}

FText UH_HUDWidget::GetObjectiveText() const
{
	if (const ASparta_HCharacter* Character = GetOwningCharacter())
	{
		return FText::FromString(Character->CurrentObjective);
	}
	return FText::GetEmpty();
}

