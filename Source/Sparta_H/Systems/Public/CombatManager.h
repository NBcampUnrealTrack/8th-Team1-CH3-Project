#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HitDetector.h"
#include "CombatTypes.h"
#include "GameFramework/Pawn.h"
#include "DamageProcessor.h"
#include "CombatManager.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPARTA_H_API UCombatManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatManager();
	
	UPROPERTY()
	UHitDetector* HitDetector;

	UPROPERTY()
	UDamageProcessor* DamageProcessor;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	float BaseDamage = 50.f;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	float TraceRange = 20000.f;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	float KnifeRange = 200.f;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	float KnifeBackDamage = 100.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float KnifeFrontDamage = 50.f;

	// 사격 처리 메인 함수
	void Fire(const FVector& AimStart, const FVector& AimDirection, ECombatWeaponType WeaponType);
	
	// 소음 발생 함수
	void EmitNoise(const FVector& NoiseLocation, float NoiseRange);
	
private:
	float GetFireNoiseRange(ECombatWeaponType WeaponType) const;
	float GetHitNoiseRange(ECombatWeaponType WeaponType) const;
	void KnifeAttack(const FVector& AimStart, const FVector& AimDirection);
};
