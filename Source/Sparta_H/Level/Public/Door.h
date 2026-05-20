#pragma once

#include "CoreMinimal.h"
#include "BaseInteractableActor.h"
#include "Door.generated.h"

UCLASS()
class SPARTA_H_API ADoor : public ABaseInteractableActor
{
    GENERATED_BODY()
    
public:    
    ADoor();

protected:
    virtual void BeginPlay() override;

public:    
    virtual void Tick(float DeltaTime) override;

    virtual void Interact_Implementation(class APlayerCharacter* Interactor) override;
    virtual bool CanInteract_Implementation(class APlayerCharacter* Interactor) const override;
    virtual FString GetInteractionText_Implementation() const override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Settings")
    float ClosedYaw = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Settings")
    float OpenedYaw = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Settings")
    float DoorSpeed = 5.0f;

    virtual void OnSensorOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
    virtual void OnSensorOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;

    void ToggleDoor();

private:
    bool bIsOpening = false;
};