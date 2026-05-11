#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MissionTrigger.generated.h"

class UBoxComponent;

/**
 * 특정 구역 진입 시 현재 미션 목표를 완료시키는 트리거 액터
 */
UCLASS()
class SPARTA_H_API AMissionTrigger : public AActor
{
	GENERATED_BODY()

public:
	AMissionTrigger();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* TriggerBox;

	// 이 트리거가 완료시킬 미션 목표의 ID (Data Asset에 정의된 GoalID와 매칭)
	// 비워둘 경우 단순히 플레이어의 '현재' 목표를 다음으로 넘깁니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FName TargetGoalID;

	// // 트리거 활성화 시 재생할 사운드
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	// class USoundBase* CompletionSound;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                    bool bFromSweep, const FHitResult& SweepResult);

private:
	bool bIsUsed = false;
};
