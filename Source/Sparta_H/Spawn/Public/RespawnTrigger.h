#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RespawnTrigger.generated.h"

class UBoxComponent;
class ABaseSpawnVolume;

UCLASS()
class SPARTA_H_API ARespawnTrigger : public AActor
{
	GENERATED_BODY()
	
public:	
	ARespawnTrigger();

	// Modified: 트리거 상태 초기화 및 중첩 확인을 위한 함수 선언 (구현부에서 상세 로직 처리)
	UFUNCTION(BlueprintCallable, Category = "Trigger")
	void ResetTrigger();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger|Settings")
	bool bHasTriggered = false;

protected:
	virtual void BeginPlay() override;

	// Modified: 실제 스폰 실행 로직을 별도 함수로 분리
	void ExecuteSpawn();
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Trigger|Components")
	TObjectPtr<UBoxComponent> TriggerBox;
	
	// 이 트리거와 연결된 스폰 볼륨들 (레벨에서 직접 할당)
	UPROPERTY(EditInstanceOnly, Category="Trigger|Settings")
	TArray<TObjectPtr<ABaseSpawnVolume>> TargetSpawnVolumes;
	
	UFUNCTION()
	void HandleOnComponentBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
};
