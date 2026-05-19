#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MissionInteractableInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable, BlueprintType)
class UMissionInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 미션 관련 상호작용이 가능한 액터들을 위한 인터페이스
 */
class SPARTA_H_API IMissionInteractableInterface
{
	GENERATED_BODY()

public:
	// 상호작용 실행 시 호출
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mission|Interaction")
	void Interact(class APlayerCharacter* Interactor);

	// 현재 상호작용 가능한 상태인지 확인
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mission|Interaction")
	bool CanInteract(class APlayerCharacter* Interactor) const;

	// 상호작용 시 표시할 텍스트
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mission|Interaction")
	FString GetInteractionText() const;
};
