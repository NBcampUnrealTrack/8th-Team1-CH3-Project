#pragma once

#include "CoreMinimal.h"
#include "BaseInteractableActor.h"
#include "FirstMissionItem.generated.h"

class APlayerCharacter;

UCLASS()
class SPARTA_H_API AFirstMissionItem : public ABaseInteractableActor
{
	GENERATED_BODY()

public:
	AFirstMissionItem();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Mission")
	FName TargetGoalID;
	
	UPROPERTY(EditAnywhere, Category = "Mission")
	bool bUnlockRifleOnInteract = false;

	bool bIsUsed = false;

public:
	virtual void Interact_Implementation(APlayerCharacter* Interactor) override;
	virtual bool CanInteract_Implementation(APlayerCharacter* Interactor) const override;
	virtual FString GetInteractionText_Implementation() const override;
};