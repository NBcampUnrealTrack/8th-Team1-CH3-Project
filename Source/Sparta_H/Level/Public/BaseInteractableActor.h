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

	// 자식 클래스에서 메인 메시로 사용할 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MainMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* GlowMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* SensorBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Materials")
	UMaterialInterface* OutlineOverlayMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Materials")
	UMaterialInterface* RedGlowMaterial;

	// 겹침 감지 이벤트
	UFUNCTION()
	virtual void OnSensorOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnSensorOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	bool bIsPlayerNearby = false;

public:
	// 인터페이스 기본 구현
	virtual void Interact_Implementation(class APlayerCharacter* Interactor) override;
	virtual bool CanInteract_Implementation(class APlayerCharacter* Interactor) const override;
	virtual FString GetInteractionText_Implementation() const override;
};