#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RooftopDoor.generated.h"

UCLASS()
class SPARTA_H_API ARooftopDoor : public AActor
{
    GENERATED_BODY()

public:
    ARooftopDoor();
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Door")
    void OpenDoor();
    virtual void OpenDoor_Implementation();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UStaticMeshComponent* DoorMesh;

    // 열릴 때 이동 방향·거리 (기본: Z축으로 300cm 들어올림)
    UPROPERTY(EditDefaultsOnly, Category = "Door")
    FVector OpenOffset = FVector(0.f, 0.f, 300.f);

    UPROPERTY(EditDefaultsOnly, Category = "Door")
    float DoorMoveSpeed = 200.f;

private:
    bool bOpening = false;
    FVector ClosedLocation;
};