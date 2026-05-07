#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseEnemy.h"
#include "AlertManager.generated.h"

// 경보 활성화 시 브로드캐스트 (UI, 사운드 등 연동용)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAlertActivatedDelegate);

UCLASS()
class SPARTA_H_API AAlertManager : public AActor
{
	GENERATED_BODY()

public:
	AAlertManager();

	// ---------------------------------------------------------------
	// 폭탄 설치 / MissionSystem에서 호출
	// 현재 레벨의 모든 적을 즉시 Combat으로 강제 전환
	// 이후 스폰되는 적은 Lost 상태로 시작
	// ---------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Alert")
	void ActivateAlert();

	// ---------------------------------------------------------------
	// 스폰 시 초기 AlertLevel 반환
	// Off → Idle(1) / On → Lost(4)
	// LevelSpawn에서 적 스폰 시 호출
	// ---------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Alert")
	EAlertLevel GetSpawnAlertLevel() const;

	UFUNCTION(BlueprintCallable, Category = "Alert")
	bool IsAlertActive() const { return bIsAlertActive; }

	// 경보 활성화 이벤트 (UI / 사운드 연동)
	UPROPERTY(BlueprintAssignable, Category = "Alert")
	FOnAlertActivatedDelegate OnAlertActivated;

	// 월드에서 AlertManager 인스턴스 검색 (싱글턴 접근용)
	UFUNCTION(BlueprintCallable, Category = "Alert", meta = (WorldContext = "WorldContextObject"))
	static AAlertManager* GetInstance(UObject* WorldContextObject);

protected:
	virtual void BeginPlay() override;

private:
	// 경보 활성화 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Alert", meta = (AllowPrivateAccess = "true"))
	bool bIsAlertActive = false;

	// 월드의 모든 적을 지정 AlertLevel로 강제 전환
	void ForceAlertLevelToAllEnemies(EAlertLevel NewLevel);
};