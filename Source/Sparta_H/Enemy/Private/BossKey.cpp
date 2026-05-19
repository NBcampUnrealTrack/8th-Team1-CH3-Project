#include "BossKey.h"
#include "RooftopDoor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

ABossKey::ABossKey()
{
    PrimaryActorTick.bCanEverTick = false;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
    PickupSphere->SetupAttachment(Mesh);
    PickupSphere->SetSphereRadius(80.f);
    PickupSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void ABossKey::BeginPlay()
{
    Super::BeginPlay();
    PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &ABossKey::OnOverlapBegin);
}

void ABossKey::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || !OtherActor->ActorHasTag(TEXT("Player"))) return;

    // 레벨에 배치된 ARooftopDoor를 자동 탐색
    ARooftopDoor* Door = Cast<ARooftopDoor>(
        UGameplayStatics::GetActorOfClass(this, ARooftopDoor::StaticClass()));

    if (IsValid(Door))
    {
        Door->OpenDoor();
    }

    Destroy();
}