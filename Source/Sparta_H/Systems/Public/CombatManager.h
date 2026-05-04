#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HitDetector.h"
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

	// 사격 처리 메인 함수
	void Fire(const FVector& AimStart, const FVector& AimDirection);
		
};
