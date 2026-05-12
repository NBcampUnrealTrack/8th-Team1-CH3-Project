#pragma once

#include "CoreMinimal.h"
#include "BaseEnemy.h"
#include "Perception/AIPerceptionTypes.h"
#include "CCTV.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UStaticMeshComponent;

UCLASS()
class SPARTA_H_API ACCTV : public ABaseEnemy
{
	GENERATED_BODY()

public:
	ACCTV();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CCTV|Mesh")
	UStaticMeshComponent* CCTVMeshComp;

protected:
	virtual void BeginPlay() override;

	// 파괴 시 Perception 비활성화 후 파괴된 메시 스폰, 경보 전파 없음
	virtual void Die() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAIPerceptionComponent* AIPerceptionComp;

	UPROPERTY()
	UAISenseConfig_Sight* SightConfig;

	UFUNCTION()
	void OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus);

private:
	// CCTV가 플레이어를 직접 목격 → 주변 적을 즉시 Combat으로 전환
	void AlertNearbyEnemies(AActor* TargetPlayer);

	UPROPERTY(EditDefaultsOnly, Category = "AI|Sight")
	float VisualFOV = 120.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Sight")
	float SightRange = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|CCTV", meta = (AllowPrivateAccess = "true"))
	float ShareRange = 3000.0f;

	// 파괴 시 스폰할 부서진 CCTV 스태틱 메시 (에디터에서 SM_CCTV_Broken 할당)
	UPROPERTY(EditDefaultsOnly, Category = "AI|CCTV")
	UStaticMesh* BrokenCCTVMesh;
};