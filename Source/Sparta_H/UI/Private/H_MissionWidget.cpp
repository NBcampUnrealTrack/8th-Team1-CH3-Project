#include "H_MissionWidget.h"

#include "MissionDataAsset.h"
#include "Components/TextBlock.h"
#include "Components/CheckBox.h"
#include "PlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Components/Image.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h" // Modified: CanvasPanelSlot 조작을 위해 추가

void UH_MissionWidget::UpdateFromCharacter(APlayerCharacter* Character)
{
	if (!Character || CachedCharacter == Character)
	{
		return;
	}

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

	if (!CachedCharacter.IsValid() || !MissionGoalText)
	{
		return;
	}

	// 1. 현재 미션 목표 데이터 가져오기
	const FMissionGoal* CurrentGoal = nullptr;
	if (CachedCharacter->CurrentMissionData &&
		CachedCharacter->CurrentMissionData->MissionGoals.IsValidIndex(CachedCharacter->CurrentMissionIndex))
	{
		CurrentGoal = &CachedCharacter->CurrentMissionData->MissionGoals[CachedCharacter->CurrentMissionIndex];
	}

	// 2. 거리 정보 및 텍스트 업데이트
	float Distance = CachedCharacter->GetDistanceToCurrentObjective();
	FString FinalText = CurrentGoalDescription;

	// Modified: 거리가 0보다 클 때 거리 텍스트 추가
	if (Distance > 0.0f)
	{
		FinalText += FString::Printf(TEXT("\n[거리: %.1fm]"), Distance);
	}
	MissionGoalText->SetText(FText::FromString(FinalText));

	// 3. Waypoint 마커 화면 표시 로직
	if (WaypointMarker && CurrentGoal)
	{
		FVector TargetLoc = CurrentGoal->TargetLocation;

		if (!TargetLoc.IsZero())
		{
			FVector2D ScreenPos;
			// Modified: bPlayerViewportRelative를 true로 설정하여 위젯 좌표계 투영 정확도 향상
			bool bIsOnScreen = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
				GetOwningPlayer(),
				TargetLoc,
				ScreenPos,
				true
			);

			if (bIsOnScreen)
			{
				WaypointMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
				
				// Modified: SetRenderTranslation 대신 CanvasPanelSlot의 SetPosition 사용 (오차 감소)
				if (UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(WaypointMarker))
				{
					CanvasSlot->SetPosition(ScreenPos);
				}

				// Modified: 목표 지점 아이콘 동적 로드 및 설정 보강
				UTexture2D* IconTex = nullptr;
				if (CurrentGoal->WaypointIcon.IsPending())
				{
					IconTex = CurrentGoal->WaypointIcon.LoadSynchronous();
				}
				else
				{
					IconTex = CurrentGoal->WaypointIcon.Get();
				}

				if (IconTex)
				{
					WaypointMarker->SetBrushFromTexture(IconTex);
				}
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

void UH_MissionWidget::HandleOnObjectiveChanged(const FString& NewObjective)
{
	UpdateMissionGoal(NewObjective, false);
}

void UH_MissionWidget::UpdateMissionGoal(const FString& GoalDescription, bool bIsCompleted)
{
	CurrentGoalDescription = GoalDescription;
	// 텍스트 업데이트는 NativeTick에서 수행됨
}
