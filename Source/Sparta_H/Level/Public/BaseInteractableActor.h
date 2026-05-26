#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MissionInteractableInterface.h"
#include "BaseInteractableActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UMaterialInterface;

UCLASS()
class SPARTA_H_API ABaseInteractableActor : public AActor, public IMissionInteractableInterface
{
	GENERATED_BODY()
    
public:    
	ABaseInteractableActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* DefaultSceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MainMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* SensorBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Materials")
	UMaterialInterface* OutlineOverlayMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Materials")
	UMaterialInterface* RedGlowMaterial;

	UFUNCTION()
	virtual void OnSensorOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnSensorOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	bool bIsPlayerNearby = false;

private:
	UPROPERTY()
	TArray<UMaterialInterface*> OriginalMaterials;

public:
	virtual void Interact_Implementation(class APlayerCharacter* Interactor) override;
	virtual bool CanInteract_Implementation(class APlayerCharacter* Interactor) const override;
	virtual FString GetInteractionText_Implementation() const override;
};