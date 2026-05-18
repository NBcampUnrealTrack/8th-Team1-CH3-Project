#pragma once

#include "CoreMinimal.h"
#include "SpawnTypes.h"
#include "GameFramework/Actor.h"
#include "BaseSpawnVolume.generated.h"

class UBoxComponent;
class APawn;

UCLASS()
class SPARTA_H_API ABaseSpawnVolume : public AActor
{
	GENERATED_BODY()

public:
	ABaseSpawnVolume();

	// 트리거에서 호출, 
	UFUNCTION(BlueprintCallable, Category="Spawn")
	void TriggerInitialSpawn();

	UFUNCTION(BlueprintCallable, Category="Spawn")
	ESpawnState GetSpawnState() const { return CurrentSpawnState; }

protected:
	virtual void BeginPlay() override;

	// ───── 컴포넌트 ─────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spawn|Components")
	TObjectPtr<UBoxComponent> SpawnArea;

	// ───── 스폰설정 ─────
	UPROPERTY(EditAnywhere, Category="Spawn|Settings")
	TSubclassOf<APawn> EnemyClass;

	UPROPERTY(EditAnywhere, Category="Spawn|Settings", meta=(ClampMin="0"))
	int32 BaseSpawnCount = 1;

	UPROPERTY(EditAnywhere, Category="Spawn|Settings", meta=(ClampMin="0"))
	int32 ExtraSpawnCount = 2;

	// Combat 상태 감지 반경
	UPROPERTY(EditAnywhere, Category="Spawn|Settings", meta=(ClampMin="0.0"))
	float CombatTriggerRadius = 500.0f;

	// NavMesh 검색 반경
	UPROPERTY(EditAnywhere, Category="Spawn|Settings", meta=(ClampMin="0.0"))
	float NavSearchRadius = 1000.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spawn|State")
	ESpawnState CurrentSpawnState = ESpawnState::Idle;

	// ───── Combat 브로드캐스트 핸들러 ─────
	UFUNCTION()
	void HandleCombatEntered(FVector CombatLocation);

private:
	// 박스 영역
	bool GetRandomSpawnLocation(FVector& OutLocation) const;

	// 실제 스폰 처리
	void SpawnEnemies(int32 Count, ESpawnState NextStateOnSuccess);
};
