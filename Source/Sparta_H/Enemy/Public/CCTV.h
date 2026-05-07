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

    // 파괴 시 Perception 비활성화 및 마지막 경보 전파
    virtual void Die() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAIPerceptionComponent* AIPerceptionComp;

    UPROPERTY()
    UAISenseConfig_Sight* SightConfig;

    UFUNCTION()
    void OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus);

private:
    // bForceMaxAlert: true이면 주변 적을 즉시 Combat으로 강제 전환 (파괴 시 사용)
    void AlertNearbyEnemies(AActor* TargetPlayer, bool bForceMaxAlert = false);

    UPROPERTY(EditDefaultsOnly, Category = "AI|Sight")
    float VisualFOV = 120.0f;

    UPROPERTY(EditDefaultsOnly, Category = "AI|Sight")
    float SightRange = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|CCTV", meta = (AllowPrivateAccess = "true"))
    float ShareRange = 3000.0f;

    // 마지막으로 감지한 플레이어 캐싱 (파괴 시 최종 경보에 사용)
    UPROPERTY()
    AActor* LastPerceivedTarget;
};