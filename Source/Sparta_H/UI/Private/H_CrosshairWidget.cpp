#include "H_CrosshairWidget.h"
#include "PlayerCharacter.h"
#include "WeaponTypes.h"

ECrosshairState UH_CrosshairWidget::GetCurrentCrosshairState() const
{
	if (APlayerCharacter* Character = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
	{
		return Character->CurrentCrosshairState;
	}
	return ECrosshairState::Default;
}

void UH_CrosshairWidget::SetCrosshairState(ECrosshairState NewState)
{
	if (APlayerCharacter* Character = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
	{
		Character->CurrentCrosshairState = NewState;
	}
}

//커밋 테스트