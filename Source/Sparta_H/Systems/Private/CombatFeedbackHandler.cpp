#include "CombatFeedbackHandler.h"

UCombatFeedbackHandler::UCombatFeedbackHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatFeedbackHandler::OnKill()
{
	OnKillDelegate.Broadcast();
}