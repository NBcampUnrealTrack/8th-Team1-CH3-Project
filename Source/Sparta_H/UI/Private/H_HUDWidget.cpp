#include "H_HUDWidget.h"
#include "PlayerCharacter.h"
#include "H_StatBarWidget.h"
#include "H_WeaponWidget.h"
#include "H_MissionWidget.h"
#include "H_TimerWidget.h"

APlayerCharacter* UH_HUDWidget::GetOwningCharacter() const
{
	return Cast<APlayerCharacter>(GetOwningPlayerPawn());
}

void UH_HUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APlayerCharacter* Character = GetOwningCharacter();
	if (!Character) return;

	if (HealthBar)
	{
		HealthBar->UpdateFromCharacter(Character, 0);
	}

	if (StaminaBar)
	{
		StaminaBar->UpdateFromCharacter(Character, 1);
	}

	if (NoiseBar)
	{
		NoiseBar->UpdateFromCharacter(Character, 2);
	}

	if (WeaponUI)
	{
		WeaponUI->UpdateFromCharacter(Character);
	}

	if (MissionUI)
	{
		MissionUI->UpdateFromCharacter(Character);
	}

	if (TimerUI)
	{
		float Time = Character->GetRemainingMissionTime();
		TimerUI->UpdateTimer(Time);
	}
}
