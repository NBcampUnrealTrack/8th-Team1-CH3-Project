#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Systems/Public/MissionInteractableInterface.h"
#include "MissionItem.generated.h"

/**
 * 연구 데이터, 인질 등 상호작용을 통해 미션을 완료시키는 오브젝트
 */
UCLASS()
class SPARTA_H_API AMissionItem : public AActor, public IMissionInteractableInterface
{
	GENERATED_BODY()
	
public:	
	AMissionItem();

	// 인터페이스 구현
	virtual void Interact_Implementation(class APlayerCharacter* Interactor) override;
	virtual bool CanInteract_Implementation(class APlayerCharacter* Interactor) const override;
	virtual FString GetInteractionText_Implementation() const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* Mesh;

	// 이 아이템이 완료시킬 미션 목표 ID
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FName TargetGoalID;

	// 상호작용 시 표시할 텍스트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FString InteractionText = TEXT("데이터 회수");

	// 상호작용 후 액터 제거 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	bool bDestroyOnInteract = true;

private:
	bool bIsUsed = false;
};
