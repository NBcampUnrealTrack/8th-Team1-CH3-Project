#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "H_MissionWidget.generated.h"

UCLASS()
class SPARTA_H_API UH_MissionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI|Mission")
	void UpdateFromCharacter(class ASparta_HCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "UI|Mission")
	void UpdateMissionGoal(const FString& GoalDescription, bool bIsCompleted);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* MissionGoalText;

};
