#include "Level/Public/BaseInteractableActor.h"
#include "PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

ABaseInteractableActor::ABaseInteractableActor()
{
    PrimaryActorTick.bCanEverTick = false;

    DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
    RootComponent = DefaultSceneRoot;

    MainMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MainMesh"));
    MainMesh->SetupAttachment(RootComponent);

    SensorBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SensorBox"));
    SensorBox->SetupAttachment(MainMesh);
    SensorBox->SetCollisionObjectType(ECC_WorldDynamic);
    SensorBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SensorBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    SensorBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ABaseInteractableActor::BeginPlay()
{
    Super::BeginPlay();

    if (SensorBox)
    {
        SensorBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ABaseInteractableActor::OnSensorOverlapBegin);
        SensorBox->OnComponentEndOverlap.AddUniqueDynamic(this, &ABaseInteractableActor::OnSensorOverlapEnd);
    }

    if (MainMesh)
    {
        MainMesh->SetOverlayMaterial(nullptr);
        
        int32 MaterialCount = MainMesh->GetNumMaterials();
        for (int32 i = 0; i < MaterialCount; ++i)
        {
            OriginalMaterials.Add(MainMesh->GetMaterial(i));
        }
    }
}

void ABaseInteractableActor::OnSensorOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor->IsA(APlayerCharacter::StaticClass()))
    {
        bIsPlayerNearby = true;
        
        if (MainMesh)
        {
            if (OutlineOverlayMaterial)
            {
                MainMesh->SetRenderCustomDepth(true);
                MainMesh->SetOverlayMaterial(OutlineOverlayMaterial);
            }
            
            if (RedGlowMaterial)
            {
                int32 MaterialCount = MainMesh->GetNumMaterials();
                for (int32 i = 0; i < MaterialCount; ++i)
                {
                    MainMesh->SetMaterial(i, RedGlowMaterial);
                }
            }
        }
    }
}

void ABaseInteractableActor::OnSensorOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (OtherActor && OtherActor->IsA(APlayerCharacter::StaticClass()))
    {
        bIsPlayerNearby = false;
        
        if (MainMesh)
        {
            MainMesh->SetRenderCustomDepth(false);
            MainMesh->SetOverlayMaterial(nullptr);
            
            for (int32 i = 0; i < OriginalMaterials.Num(); ++i)
            {
                if (OriginalMaterials[i])
                {
                    MainMesh->SetMaterial(i, OriginalMaterials[i]);
                }
            }
        }
    }
}

void ABaseInteractableActor::Interact_Implementation(APlayerCharacter* Interactor) { }
bool ABaseInteractableActor::CanInteract_Implementation(APlayerCharacter* Interactor) const { return bIsPlayerNearby; }
FString ABaseInteractableActor::GetInteractionText_Implementation() const { return TEXT("상호작용 (F)"); }