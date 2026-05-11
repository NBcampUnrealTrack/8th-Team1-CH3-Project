#pragma once

#include "CoreMinimal.h"
#include "MissionTypes.generated.h"

// 개별 미션 목표를 정의하는 구조체
USTRUCT(BlueprintType)
struct FMissionGoal
{
	GENERATED_BODY()

	// 미션 목표 ID (시스템 식별용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FName GoalID;

	// 플레이어에게 표시될 설명
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FString Description;

	// 목표 지점 위치 (나침반이나 거리 표시에 활용 가능)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FVector TargetLocation = FVector::ZeroVector;

	// 목표 수행 시간 제한 (초 단위, 0이면 무제한)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	float TimeLimit = 0.0f;

	// 목표 지점에 표시할 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	TSoftObjectPtr<class UTexture2D> WaypointIcon;

	// 목표 완료 시 재생할사운드
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	class USoundBase* GoalCompletionSound;

	// 목표 완료 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	bool bIsOptional = false;
};


UENUM(BlueprintType)
enum class EMissionStatus : uint8
{
	NotStarted,
	InProgress,
	Completed,
	Failed
};
