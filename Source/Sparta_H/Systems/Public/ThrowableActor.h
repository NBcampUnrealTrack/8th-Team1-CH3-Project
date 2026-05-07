#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "ThrowableActor.generated.h"

UCLASS()
class SPARTA_H_API AThrowableActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AThrowableActor();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleOnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse,
		const FHitResult& Hit);
	
public:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovement;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	float ExplosionRadius = 600.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ExplosionDamage = 150.f;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	ECombatWeaponType ThrowableType = ECombatWeaponType::Grenade;
	
	void Launch(const FVector& Direction);


};
