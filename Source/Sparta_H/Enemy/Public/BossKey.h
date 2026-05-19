#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossKey.generated.h"

class USphereComponent;

UCLASS()
class SPARTA_H_API ABossKey : public AActor
{
    GENERATED_BODY()

public:
    ABossKey();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UStaticMeshComponent* Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    USphereComponent* PickupSphere;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);
};