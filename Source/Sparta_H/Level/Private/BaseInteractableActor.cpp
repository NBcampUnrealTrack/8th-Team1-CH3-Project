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

    GlowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GlowMesh"));
    GlowMesh->SetupAttachment(MainMesh);
    GlowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GlowMesh->SetHiddenInGame(true);

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

    // 겹침 이벤트 바인딩
    if (SensorBox)
    {
        SensorBox->OnComponentBeginOverlap.AddDynamic(this, &ABaseInteractableActor::OnSensorOverlapBegin);
        SensorBox->OnComponentEndOverlap.AddDynamic(this, &ABaseInteractableActor::OnSensorOverlapEnd);
    }

    // 초기 머티리얼 세팅
    if (MainMesh && GlowMesh)
    {
        MainMesh->SetOverlayMaterial(nullptr);
        UStaticMesh* TargetMesh = MainMesh->GetStaticMesh();
        
        if (TargetMesh)
        {
            GlowMesh->SetStaticMesh(TargetMesh);
            if (RedGlowMaterial)
            {
                int32 MaterialCount = GlowMesh->GetNumMaterials();
                for (int32 i = 0; i < MaterialCount; ++i)
                {
                    GlowMesh->SetMaterial(i, RedGlowMaterial);
                }
            }
        }
    }
}

void ABaseInteractableActor::OnSensorOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor->IsA(APlayerCharacter::StaticClass()))
    {
        bIsPlayerNearby = true;
        if (MainMesh && OutlineOverlayMaterial)
        {
            MainMesh->SetRenderCustomDepth(true);
            MainMesh->SetOverlayMaterial(OutlineOverlayMaterial);
        }
        if (GlowMesh)
        {
            GlowMesh->SetHiddenInGame(false);
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
        }
        if (GlowMesh)
        {
            GlowMesh->SetHiddenInGame(true);
        }
    }
}

void ABaseInteractableActor::Interact_Implementation(APlayerCharacter* Interactor) { }
bool ABaseInteractableActor::CanInteract_Implementation(APlayerCharacter* Interactor) const { return bIsPlayerNearby; }
FString ABaseInteractableActor::GetInteractionText_Implementation() const { return TEXT("상호작용 (F)"); }