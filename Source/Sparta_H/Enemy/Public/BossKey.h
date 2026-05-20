#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossKey.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class USoundBase;

UCLASS()
class SPARTA_H_API ABossKey : public AActor
{
    GENERATED_BODY()

public:
    ABossKey();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* PickupSphere;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BossKey|Settings")
    float RotationSpeed = 120.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BossKey|Settings")
    float BobbingAmplitude = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BossKey|Settings")
    float BobbingFrequency = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BossKey|Settings")
    USoundBase* PickupSound;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

private:
    FVector InitialLocation;
};