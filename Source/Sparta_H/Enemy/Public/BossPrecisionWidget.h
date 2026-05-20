#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BossPrecisionWidget.generated.h"

class UImage;

/**
 * 보스 정밀 조준 경고 위젯
 *
 * 사용법:
 * 1. 이 클래스를 부모로 WBP_BossPrecisionWarning 블루프린트 생성
 * 2. WBP 디자이너에서 Image 위젯을 배치하고 이름을 "WarningImage"로 설정
 * 3. WarningImage의 Brush > Image에 원하는 텍스처/이미지 지정
 * 4. (선택) OnWarningStarted / OnWarningStopped 이벤트로 애니메이션 추가
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

	// C++/BP에서 런타임에 이미지 교체하고 싶을 때 사용
	UFUNCTION(BlueprintCallable, Category = "Boss|UI")
	void SetWarningImage(UTexture2D* Texture);

protected:
	// WBP 디자이너에서 "WarningImage"라는 이름의 Image 위젯과 자동 바인딩
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> WarningImage;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|UI")
	void OnWarningStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|UI")
	void OnWarningStopped();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|UI")
	void OnWarningProgressUpdated(float Progress);
};