#pragma once

#include "CoreMinimal.h"
#include "BaseEnemy.h"
#include "Perception/AIPerceptionTypes.h"
#include "CCTV.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;

UCLASS()
class SPARTA_H_API ACCTV : public ABaseEnemy
{
    GENERATED_BODY()

public:
    ACCTV();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAIPerceptionComponent* AIPerceptionComp;

    UPROPERTY()
    UAISenseConfig_Sight* SightConfig;

    UFUNCTION()
    void OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus);

private:
    void AlertNearbyEnemies(AActor* TargetPlayer);

    UPROPERTY(EditDefaultsOnly, Category = "AI|Sight")
    float VisualFOV = 60.0f;

    UPROPERTY(EditDefaultsOnly, Category = "AI|Sight")
    float SightRange = 1200.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|CCTV", meta = (AllowPrivateAccess = "true"))
    float ShareRange = 2000.0f; // 경보를 전파할 주변 범위 (20미터)
};