#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "ThrowableActor.generated.h"

class UNiagaraSystem;

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

	UPROPERTY(EditAnywhere, Category = "Combat|Noise")
	float RockNoiseRange = 2000.f;

	// 폭발 Niagara 이펙트 (BP에서 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Combat|VFX")
	UNiagaraSystem* ExplosionEffect;

	// 폭발 사운드 (BP에서 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Combat|VFX")
	USoundBase* ExplosionSound;

	// 카메라 방향과 차징 속도를 받아 Velocity 세팅. Speed <= 0 이면 ProjectileMovement->InitialSpeed 폴백
	void Launch(const FVector& Direction, float Speed = 0.0f);
};
