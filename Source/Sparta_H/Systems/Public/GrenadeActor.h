#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GrenadeActor.generated.h"

UCLASS()
class SPARTA_H_API AGrenadeActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AGrenadeActor();

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
	float ExplosionRadius = 200.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ExplosionDamage = 150.f;
	
	void Launch(const FVector& Direction);


};
