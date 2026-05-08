#include "H_CrosshairWidget.h"
#include "../Characters/PlayerCharacter.h"

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
