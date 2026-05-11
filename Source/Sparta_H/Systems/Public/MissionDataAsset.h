#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MissionTypes.h"
#include "MissionDataAsset.generated.h"

UCLASS(BlueprintType)
class SPARTA_H_API UMissionDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 미션 고유 ID
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FName MissionID;

	// 미션 데이터 에셋의 이름 또는 제목
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FString MissionTitle;

	// 순차적으로 진행될 미션 목표 리스트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	TArray<FMissionGoal> MissionGoals;
};
