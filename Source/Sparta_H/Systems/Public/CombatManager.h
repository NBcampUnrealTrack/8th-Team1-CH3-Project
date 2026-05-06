#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HitDetector.h"
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
	float FireNoiseRange = 30000.f;  // 발사 소음 300m

	UPROPERTY(EditAnywhere, Category = "Combat")
	float HitNoiseRange = 2000.f;    // 피격 소음 20m

	// 사격 처리 메인 함수
	void Fire(const FVector& AimStart, const FVector& AimDirection);
	
	// 소음 발생 함수
	void EmitNoise(const FVector& NoiseLocation, float NoiseRange);
		
};
