#include "H_HUDWidget.h"
#include "../Sparta_HCharacter.h"
#include "H_StatBarWidget.h"
#include "H_WeaponWidget.h"
#include "H_MissionWidget.h"

APlayerCharacter* UH_HUDWidget::GetOwningCharacter() const
{
	return Cast<APlayerCharacter>(GetOwningPlayerPawn());
}

void UH_HUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ASparta_HCharacter* Character = GetOwningCharacter();
	if (!Character) return;

	if (HealthBar)
	{
		HealthBar->UpdateFromCharacter(Character, true);
	}

	if (StaminaBar)
	{
		StaminaBar->UpdateFromCharacter(Character, false);
	}

	if (WeaponUI)
	{
		WeaponUI->UpdateFromCharacter(Character);
	}

	if (MissionUI)
	{
		MissionUI->UpdateFromCharacter(Character);
	}
}
