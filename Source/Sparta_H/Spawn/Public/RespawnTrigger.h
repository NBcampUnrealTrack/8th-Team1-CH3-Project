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

	// 트리거 상태 초기화 (다시하기 시 호출)
	UFUNCTION(BlueprintCallable, Category = "Trigger")
	void ResetTrigger() { bHasTriggered = false; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger|Settings")
	bool bHasTriggered = false;

protected:
	virtual void BeginPlay() override;
	
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
