#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BossPrecisionWidget.generated.h"

/**
 * 보스 정밀 조준 경고 위젯
 *
 * 사용법:
 * 1. 이 클래스를 부모로 WBP_BossPrecisionWarning 블루프린트 생성
 * 2. WBP 내에서 OnWarningStarted / OnWarningStopped 이벤트 구현
 *    - OnWarningStarted → 깜빡임/레드 조준 애니메이션 재생
 *    - OnWarningStopped → 애니메이션 정지, 위젯 숨김 처리
 */
UCLASS()
class SPARTA_H_API UBossPrecisionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// BossEnemy::StartPrecisionAim()에서 호출 — 조준 경고 시작
	UFUNCTION(BlueprintCallable, Category = "Boss|UI")
	void StartWarning();

	// BossEnemy::ExecutePrecisionFire()에서 호출 — 조준 경고 종료
	UFUNCTION(BlueprintCallable, Category = "Boss|UI")
	void StopWarning();

	// 조준 경고 남은 시간 (0~1 비율)을 매 틱 블루프린트에 전달
	UFUNCTION(BlueprintCallable, Category = "Boss|UI")
	void UpdateWarningProgress(float Progress);

protected:
	// 블루프린트에서 깜빡임 애니메이션 등 시각 처리 구현
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|UI")
	void OnWarningStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|UI")
	void OnWarningStopped();

	// 조준 진행도 (0=시작, 1=발사 직전) — 블루프린트에서 프로그레스 바 등에 활용
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|UI")
	void OnWarningProgressUpdated(float Progress);
};