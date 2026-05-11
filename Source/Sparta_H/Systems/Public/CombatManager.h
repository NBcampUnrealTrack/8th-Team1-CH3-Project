#pragma once

#include "CoreMinimal.h"
#include "CombatFeedbackHandler.h"
#include "Components/ActorComponent.h"
#include "HitDetector.h"
#include "CombatTypes.h"
#include "GameFramework/Pawn.h"
#include "DamageProcessor.h"
#include "CombatManager.generated.h"

class UNiagaraSystem;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SPARTA_H_API UCombatManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatManager();

	UPROPERTY()
	UHitDetector* HitDetector;

	UPROPERTY()
	UDamageProcessor* DamageProcessor;

	UPROPERTY()
	UCombatFeedbackHandler* FeedbackHandler;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float TraceRange = 20000.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float KnifeRange = 200.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float KnifeBackDamage = 100.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float KnifeFrontDamage = 50.f;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	float BackAttackThreshold = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Combat|Noise")
	float HitNoiseRangePistol = 200.f; // 2m

	UPROPERTY(EditAnywhere, Category = "Combat|Noise")
	float HitNoiseRangeRock = 500.f; // 5m
	
	UPROPERTY(EditAnywhere, Category = "Combat|Noise")
	float FireNoiseRangePistol = 30000.f; // 300m
	
	UPROPERTY(EditAnywhere, Category = "Combat|Noise")
	float FireNoiseRangeRifle = 150000.f; // 1500m
	
	UPROPERTY(EditAnywhere, Category = "Combat|Noise")
	float FireNoiseRangeKnife = 1000.f; // 10m

	// 사격 처리 메인 함수. bTriggerAIAggro=false면 발사/피격 소음을 발생시키지 않음(소음 무기 등)
	void OnFire(const FVector& AimStart, const FVector& AimDirection, ECombatWeaponType WeaponType,
	            float BaseDamage, bool bTriggerAIAggro, UNiagaraSystem* ImpactVFX = nullptr);

	// 소음 발생 함수
	void EmitNoise(const FVector& NoiseLocation, float NoiseRange);

private:
	float GetFireNoiseRange(ECombatWeaponType WeaponType) const;
	float GetHitNoiseRange(ECombatWeaponType WeaponType) const;

	// 근접(칼) 처리 — 매니저 자체의 KnifeFrontDamage/KnifeBackDamage를 사용해 백/정면 데미지 차등
	void KnifeAttack(const FVector& AimStart, const FVector& AimDirection, bool bTriggerAIAggro);
	void RegisterKillFeedback(AActor* HitActor);
};
