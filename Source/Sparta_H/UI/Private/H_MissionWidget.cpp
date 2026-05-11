#include "H_MissionWidget.h"

#include "MissionDataAsset.h"
#include "Components/TextBlock.h"
#include "Components/CheckBox.h"
#include "PlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Components/Image.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UH_MissionWidget::UpdateFromCharacter(APlayerCharacter* Character)
{
	if (!Character || CachedCharacter == Character) return;

	// 이전 바인딩 제거
	if (CachedCharacter.IsValid())
	{
		CachedCharacter->OnObjectiveChanged.RemoveDynamic(this, &UH_MissionWidget::HandleOnObjectiveChanged);
	}

	CachedCharacter = Character;
	
	// 새 바인딩
	Character->OnObjectiveChanged.AddDynamic(this, &UH_MissionWidget::HandleOnObjectiveChanged);

	// 초기 상태 설정
	UpdateMissionGoal(Character->CurrentObjective, false);
}

void UH_MissionWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!CachedCharacter.IsValid() || !MissionGoalText) return;

	// 1. 거리 정보 업데이트 (텍스트)
	float Distance = CachedCharacter->GetDistanceToCurrentObjective();
	FString FinalText = CurrentGoalDescription;
	
	if (Distance > 0.1f)
	{
		FinalText += FString::Printf(TEXT("\n[목표 거리: %.1fm]"), Distance);
	}
	MissionGoalText->SetText(FText::FromString(FinalText));

	// 2. Waypoint 마커 화면 표시 로직
	if (WaypointMarker)
	{
		// 데이터에서 위치 가져오기
		if (CachedCharacter->CurrentMissionData && 
		    CachedCharacter->CurrentMissionData->MissionGoals.IsValidIndex(CachedCharacter->CurrentMissionIndex))
		{
			FVector TargetLoc = CachedCharacter->CurrentMissionData->MissionGoals[CachedCharacter->CurrentMissionIndex].TargetLocation;
			
			if (!TargetLoc.IsZero())
			{
				FVector2D ScreenPos;
				if (UGameplayStatics::ProjectWorldToScreen(GetOwningPlayer(), TargetLoc, ScreenPos))
				{
					WaypointMarker->SetVisibility(ESlateVisibility::Visible);
					WaypointMarker->SetRenderTranslation(ScreenPos);
				}
				else
				{
					WaypointMarker->SetVisibility(ESlateVisibility::Hidden);
				}
			}
			else
			{
				WaypointMarker->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

void UH_MissionWidget::HandleOnObjectiveChanged(const FString& NewObjective)
{
	UpdateMissionGoal(NewObjective, false);
}

void UH_MissionWidget::UpdateMissionGoal(const FString& GoalDescription, bool bIsCompleted)
{
	CurrentGoalDescription = GoalDescription;
	// 텍스트 업데이트는 NativeTick에서 수행됨
}
