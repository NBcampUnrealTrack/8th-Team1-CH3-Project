#pragma once

#include "CoreMinimal.h"
#include "BaseEnemy.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyCharacter.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;

UCLASS()
class SPARTA_H_API AEnemyCharacter : public ABaseEnemy
{
    GENERATED_BODY()

public:
    AEnemyCharacter();

    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    bool CanShootTarget(AActor* TargetActor);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAIPerceptionComponent* AIPerceptionComp;

    UPROPERTY()
    UAISenseConfig_Sight* SightConfig;

    UPROPERTY()
    UAISenseConfig_Hearing* HearingConfig;

    UFUNCTION()
    void OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus);

private:
  
    UPROPERTY(EditDefaultsOnly, Category = "AI|Sight")
    float VisualFOV = 90.0f;

    UPROPERTY(EditDefaultsOnly, Category = "AI|Sight")
    float SightRange = 1500.0f;

    UPROPERTY(EditDefaultsOnly, Category = "AI|Hearing")
    float HearingRange = 150000.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat", meta = (AllowPrivateAccess = "true"))
    float FireRange = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat", meta = (AllowPrivateAccess = "true"))
    float FireAngleLimit = 30.0f;
    
    FTimerHandle DetectionTimerHandle;

    UPROPERTY()
    AActor* SuspectedTarget = nullptr;

    void OnDetectionTimerExpired();
};