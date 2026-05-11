#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "H_MissionWidget.generated.h"

class UTextBlock;
class APlayerCharacter;

UCLASS()
class SPARTA_H_API UH_MissionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI|Mission")
	void UpdateFromCharacter(APlayerCharacter* Character);

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "UI|Mission")
	void UpdateMissionGoal(const FString& GoalDescription, bool bIsCompleted);

protected:
	UFUNCTION()
	void HandleOnObjectiveChanged(const FString& NewObjective);

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* MissionGoalText;

	// 목표 지점 표시용 아이콘 (블루프린트에서 배치)
	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* WaypointMarker;

private:
	TWeakObjectPtr<APlayerCharacter> CachedCharacter;
	FString CurrentGoalDescription;
};
