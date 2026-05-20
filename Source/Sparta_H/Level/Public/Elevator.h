#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MissionInteractableInterface.h"
#include "Elevator.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EElevatorState : uint8
{
    Idle, Opening, Open, Closing, Teleporting
};

UCLASS()
class SPARTA_H_API AElevator : public AActor, public IMissionInteractableInterface
{
    GENERATED_BODY()
    
public:    
    AElevator();
    virtual void Tick(float DeltaTime) override;
    virtual void Interact_Implementation(class APlayerCharacter* Interactor) override;
    virtual bool CanInteract_Implementation(class APlayerCharacter* Interactor) const override;
    virtual FString GetInteractionText_Implementation() const override;
    
    void OpenDoorsAfterArrival();

protected:
    virtual void BeginPlay() override;
    void UpdateDoorPositions();
    void OnInteractKeyPressed();

    UFUNCTION()
    void OnOuterOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    UFUNCTION()
    void OnOuterOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
    UFUNCTION()
    void OnInnerOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    UFUNCTION()
    void OnInnerOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
    void ExecuteTeleport();
    void FinishMovement();
    
    UPROPERTY(EditAnywhere, Category = "Elevator|Effects")
    UMaterialInterface* OutlineOverlayMaterial;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    USceneComponent* Root;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* ElevatorMesh;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* OuterPanel;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* InnerPanel;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UBoxComponent* OuterSensor;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UBoxComponent* InnerSensor;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* LeftDoor;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* RightDoor;

    UPROPERTY(EditAnywhere, Category = "Elevator|Settings")
    AElevator* DestinationElevator;

    UPROPERTY(EditAnywhere, Category = "Elevator|Settings")
    float DoorMoveDistance = 129.1f;

    UPROPERTY(EditAnywhere, Category = "Elevator|Settings")
    float DoorSpeed = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator|Settings")
    float TravelDelay = 4.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator|Effects")
    TSubclassOf<class UCameraShakeBase> ElevatorShakeClass;

    UPROPERTY(EditAnywhere, Category = "Elevator|Sound")
    USoundBase* TeleportSound;

    UPROPERTY(EditAnywhere, Category = "Elevator|Sound")
    float TeleportVolume = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Elevator|Sound")
    USoundBase* ArrivalSound;

    UPROPERTY(EditAnywhere, Category = "Elevator|Sound")
    float ArrivalVolume = 0.3f;

    EElevatorState CurrentState = EElevatorState::Idle;
    float DoorAlpha = 0.0f;
    float TargetDoorAlpha = 0.0f;
    
    FVector LeftDoorInitialLoc;
    FVector RightDoorInitialLoc;

    UPROPERTY()
    class APlayerCharacter* SavedInteractor;

    bool bIsPlayerInOuter = false;
    bool bIsPlayerInInner = false;
};