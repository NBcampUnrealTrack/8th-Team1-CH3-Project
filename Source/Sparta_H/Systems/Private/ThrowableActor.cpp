#include "ThrowableActor.h"
#include "Kismet/GameplayStatics.h"

AThrowableActor::AThrowableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetNotifyRigidBodyCollision(true); 

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 1500.f;
	ProjectileMovement->MaxSpeed = 1500.f;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 1.0f;
}

void AThrowableActor::BeginPlay()
{
	Super::BeginPlay();
	MeshComponent->OnComponentHit.AddDynamic(this, &AThrowableActor::HandleOnHit);
}

void AThrowableActor::HandleOnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ThrowableType == ECombatWeaponType::Grenade)
	{
		UGameplayStatics::ApplyRadialDamage(
			GetWorld(),
			ExplosionDamage,
			GetActorLocation(),
			ExplosionRadius,
			nullptr,
			TArray<AActor*>(),
			this,
			nullptr
		);
	}
	else if (ThrowableType == ECombatWeaponType::Rock)
	{
		if (IsValid(OtherActor))
		{
			UGameplayStatics::ApplyDamage(
				OtherActor, 1.f, nullptr, this, nullptr
			);
		}
	}

	Destroy();
}
void AThrowableActor::Launch(const FVector& Direction)
{
	ProjectileMovement->Velocity = Direction * ProjectileMovement->InitialSpeed;
}