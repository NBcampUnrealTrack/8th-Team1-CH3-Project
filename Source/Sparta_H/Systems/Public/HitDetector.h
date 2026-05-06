#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatTypes.h"
#include "HitDetector.generated.h"



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPARTA_H_API UHitDetector : public UActorComponent
{
	GENERATED_BODY()

public:	
	UHitDetector();
	
	bool PerformLineTrace(const FVector& StartLocation, const FVector& Direction,float Distance,FHitResult& OutHitResult);
	
	static EHitBone IdentifyHitBone(FName BoneName);
		
};
