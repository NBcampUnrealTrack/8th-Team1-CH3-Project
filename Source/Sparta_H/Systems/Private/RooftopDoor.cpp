#include "RooftopDoor.h"
#include "Components/StaticMeshComponent.h"

ARooftopDoor::ARooftopDoor()
{
    PrimaryActorTick.bCanEverTick = false;

    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    RootComponent = DoorMesh;
}

void ARooftopDoor::BeginPlay()
{
    Super::BeginPlay();
    ClosedLocation = GetActorLocation();
}

void ARooftopDoor::OpenDoor_Implementation()
{
    if (bOpening) return;
    bOpening = true;
    DoorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetActorTickEnabled(true);
}

void ARooftopDoor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!bOpening) return;

    const FVector Target = ClosedLocation + OpenOffset;
    const FVector NewLoc = FMath::VInterpConstantTo(GetActorLocation(), Target, DeltaTime, DoorMoveSpeed);
    SetActorLocation(NewLoc);

    if (NewLoc.Equals(Target, 1.f))
    {
        bOpening = false;
        SetActorTickEnabled(false);
    }
}