#include "H_MissionWidget.h"
#include "Components/TextBlock.h"
#include "Components/CheckBox.h"
#include "../Characters/PlayerCharacter.h"

void UH_MissionWidget::UpdateFromCharacter(APlayerCharacter* Character)
{
	if (!Character) return;

	// 캐릭터의 CurrentObjective 데이터를 사용하여 업데이트
	UpdateMissionGoal(Character->CurrentObjective, false);
}

void UH_MissionWidget::UpdateMissionGoal(const FString& GoalDescription, bool bIsCompleted)
{
	if (MissionGoalText)
	{
		MissionGoalText->SetText(FText::FromString(GoalDescription));
	}
}
