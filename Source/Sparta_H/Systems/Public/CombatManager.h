#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatTypes.h"
#include "DamageDataAsset.h"
#include "CombatManager.generated.h"

class UHitDetector;
class UDamageProcessor;
class UCombatFeedbackHandler;
class UNiagaraSystem;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SPARTA_H_API UCombatManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatManager();

	virtual void InitializeComponent() override;

	UPROPERTY()
	UHitDetector* HitDetector;

	UPROPERTY()
	UDamageProcessor* DamageProcessor;

	UPROPERTY()
	UCombatFeedbackHandler* FeedbackHandler;

	UPROPERTY(EditAnywhere, Category = "Combat|Damage")
	TObjectPtr<UDamageDataAsset> RifleData;

	UPROPERTY(EditAnywhere, Category = "Combat|Damage")
	TObjectPtr<UDamageDataAsset> PistolData;

	UPROPERTY(EditAnywhere, Category = "Combat|Damage")
	float BoneHead = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Combat|Damage")
	float BoneTorso = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Combat|Damage")
	float BoneLimb = 0.7f;

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
	float HitNoiseRangeRock = 500.f; // 5m

	UPROPERTY(EditAnywhere, Category = "Combat|Noise")
	float FireNoiseRangePistol = 3000.f; // 30m

	UPROPERTY(EditAnywhere, Category = "Combat|Noise")
	float FireNoiseRangeRifle = 15000.f; // 150m

	UPROPERTY(EditAnywhere, Category = "Combat|Noise")
	float FireNoiseRangeKnife = 10.f; // 0.1m

	// 사격 처리 메인 함수. bTriggerAIAggro=false면 발사/피격 소음을 발생시키지 않음(소음 무기 등)
	// SoundRange > 0이면 해당 값 사용, 0이면 WeaponType별 기본값 사용
	void OnFire(const FVector& AimStart, const FVector& AimDirection, ECombatWeaponType WeaponType,
	            float BaseDamage, bool bTriggerAIAggro, UNiagaraSystem* ImpactVFX = nullptr, float SoundRange = 0.f);

	// 소음 발생 함수
	void EmitNoise(const FVector& NoiseLocation, float NoiseRange);

private:
	float GetFireNoiseRange(ECombatWeaponType WeaponType) const;
	float GetHitNoiseRange(ECombatWeaponType WeaponType) const;

	// 근접(칼) 처리 — 매니저 자체의 KnifeFrontDamage/KnifeBackDamage를 사용해 백/정면 데미지 차등
	void KnifeAttack(const FVector& AimStart, const FVector& AimDirection, bool bTriggerAIAggro);
	void RegisterKillFeedback(AActor* HitActor);
};
