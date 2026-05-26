#include "Enemy/Public/BossKey.h"

#include "PlayerCharacter.h"
#include "Systems/Public/RooftopDoor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

ABossKey::ABossKey()
{
	PrimaryActorTick.bCanEverTick = true;

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

	InitialLocation = GetActorLocation();
}

void ABossKey::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AddActorLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));

	float RunningTime = GetWorld()->GetTimeSeconds();
	FVector CurrentLocation = GetActorLocation();
	
	CurrentLocation.Z = InitialLocation.Z + (FMath::Sin(RunningTime * BobbingFrequency) * BobbingAmplitude);
	
	SetActorLocation(CurrentLocation);
}

void ABossKey::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || !OtherActor->ActorHasTag(TEXT("Player"))) return;

	if (APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor))
	{
		Player->bHasBossKey = true;
		UE_LOG(LogTemp, Log, TEXT("BossKey: 플레이어가 열쇠를 획득했습니다."));
	}

	ARooftopDoor* Door = Cast<ARooftopDoor>(
		UGameplayStatics::GetActorOfClass(this, ARooftopDoor::StaticClass()));

	if (IsValid(Door))
	{
		Door->OpenDoor();
	}

	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
	}

	Destroy();
}