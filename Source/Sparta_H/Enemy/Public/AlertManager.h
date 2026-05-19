#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseEnemy.h"
#include "AlertManager.generated.h"

// 경보 활성화 시 브로드캐스트 (UI, 사운드 등 연동용)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAlertActivatedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatEnteredDelegate, FVector, CombatLocation);

UCLASS()
class SPARTA_H_API AAlertManager : public AActor
{
	GENERATED_BODY()

public:
	AAlertManager();

	// ---------------------------------------------------------------
	// 폭탄 설치 / MissionSystem에서 호출
	// 폭탄 설치 지점 기준 BOMB_ALERT_RADIUS 반경 내 적을 Lost로 강제 전환
	// 이후 스폰되는 적은 Lost 상태로 시작
	// ---------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Alert")
	void ActivateAlert(FVector BombLocation);

	// ---------------------------------------------------------------
	// 스폰 시 초기 AlertLevel 반환
	// Off → Idle(1) / On → Lost(4)
	// LevelSpawn에서 적 스폰 시 호출
	// ---------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Alert")
	EAlertLevel GetSpawnAlertLevel() const;

	UFUNCTION(BlueprintCallable, Category = "Alert")
	bool IsAlertActive() const { return bIsAlertActive; }

	// 월드에서 AlertManager 인스턴스 검색 (싱글턴 접근용)
	UFUNCTION(BlueprintCallable, Category = "Alert", meta = (WorldContext = "WorldContextObject"))
	static AAlertManager* GetInstance(UObject* WorldContextObject);
	
	// 경보 활성화 이벤트 (UI / 사운드 연동)
	UPROPERTY(BlueprintAssignable, Category = "Alert")
	FOnAlertActivatedDelegate OnAlertActivated;

	// 폭탄 설치 지점 기준 고정 반경 (추후 조정)
	UPROPERTY(EditAnywhere,Category = "Alert")
	float BombAlertRadius;
	
	UPROPERTY(BlueprintAssignable, Category = "Alert")
	FOnCombatEnteredDelegate OnCombatEntered;

	UFUNCTION(BlueprintCallable, Category = "Alert")
	void NotifyCombatEntered(FVector CombatLocation);
	
protected:
	virtual void BeginPlay() override;

private:

	// 경보 활성화 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Alert", meta = (AllowPrivateAccess = "true"))
	bool bIsAlertActive = false;

	// 폭탄 설치 지점 기준 반경 내 적만 강제 전환
	void ForceAlertLevelToNearbyEnemies(EAlertLevel NewLevel, FVector Origin, float Radius);
};