#include "H_CrosshairWidget.h"
#include "../Sparta_HCharacter.h"

ECrosshairState UH_CrosshairWidget::GetCurrentCrosshairState() const
{
	if (ASparta_HCharacter* Character = Cast<ASparta_HCharacter>(GetOwningPlayerPawn()))
	{
		return Character->CurrentCrosshairState;
	}
	return ECrosshairState::Default;
}

void UH_CrosshairWidget::SetCrosshairState(ECrosshairState NewState)
{
	if (ASparta_HCharacter* Character = Cast<ASparta_HCharacter>(GetOwningPlayerPawn()))
	{
		Character->CurrentCrosshairState = NewState;
	}
}
