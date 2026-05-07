#include "ThrowableActor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"

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
		// 범위 안 액터 탐색
		TArray<FOverlapResult> Overlaps;
		FCollisionShape Sphere = FCollisionShape::MakeSphere(ExplosionRadius);
		GetWorld()->OverlapMultiByChannel(
			Overlaps,
			GetActorLocation(),
			FQuat::Identity,
			ECC_Pawn,
			Sphere
		);

		for (FOverlapResult& Overlap : Overlaps)
		{
			AActor* Target = Overlap.GetActor();
			if (!IsValid(Target)) continue;
			if (!Target->ActorHasTag("Enemy")) continue;

			const float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation()) / 100.f;

			float Multiplier = 0.f;
			if      (Distance <= 2.f) Multiplier = 1.0f;
			else if (Distance <= 4.f) Multiplier = 0.7f;
			else if (Distance <= 6.f) Multiplier = 0.5f;
			else                      Multiplier = 0.f;

			if (Multiplier <= 0.f) continue;

			const float FinalDamage = ExplosionDamage * Multiplier;
			UGameplayStatics::ApplyDamage(Target, FinalDamage, nullptr, this, nullptr);
		}
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