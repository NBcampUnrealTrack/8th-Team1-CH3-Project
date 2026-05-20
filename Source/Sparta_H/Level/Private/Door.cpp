#include "Level/Public/Door.h"
#include "PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

ADoor::ADoor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ADoor::BeginPlay()
{
    Super::BeginPlay();

    if (MainMesh)
    {
        MainMesh->SetRelativeRotation(FRotator(0.f, ClosedYaw, 0.f));
        
        // GlowMesh를 문(MainMesh)에 강제 부착
        if (GlowMesh)
        {
            GlowMesh->AttachToComponent(MainMesh, FAttachmentTransformRules::SnapToTargetIncludingScale);
            GlowMesh->SetStaticMesh(MainMesh->GetStaticMesh());
            GlowMesh->SetHiddenInGame(true);
            if (RedGlowMaterial)
            {
                for (int32 i = 0; i < GlowMesh->GetNumMaterials(); ++i)
                    GlowMesh->SetMaterial(i, RedGlowMaterial);
            }
        }

        // SensorBox를 문(MainMesh)에 강제 부착
        if (SensorBox)
        {
            SensorBox->AttachToComponent(MainMesh, FAttachmentTransformRules::KeepRelativeTransform);
        }
    }

    SetActorTickEnabled(false);
}

void ADoor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    float TargetYaw = bIsOpening ? OpenedYaw : ClosedYaw;
    FRotator CurrentRotation = MainMesh->GetRelativeRotation();
    FRotator TargetRotation = FRotator(0.f, TargetYaw, 0.f);

    if (!CurrentRotation.Equals(TargetRotation, 0.1f))
    {
        MainMesh->SetRelativeRotation(FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, DoorSpeed));
    }
    else
    {
        SetActorTickEnabled(false);
    }
}

void ADoor::OnSensorOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    Super::OnSensorOverlapBegin(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

void ADoor::OnSensorOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    Super::OnSensorOverlapEnd(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex);
}

void ADoor::Interact_Implementation(APlayerCharacter* Interactor) 
{ 
    ToggleDoor(); 
}

bool ADoor::CanInteract_Implementation(APlayerCharacter* Interactor) const 
{ 
    return true; 
}

FString ADoor::GetInteractionText_Implementation() const 
{ 
    return TEXT("상호작용 (F)"); 
}

void ADoor::ToggleDoor() 
{ 
    bIsOpening = !bIsOpening; 
    SetActorTickEnabled(true); 
}