#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeaponTypes.h"
#include "H_HUDWidget.generated.h"

class APlayerCharacter;

UCLASS()
class SPARTA_H_API UH_HUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// NativeTick에서 하위 위젯들을 실시간 업데이트
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	// Modified: 블루프린트에서 배치한 하위 위젯들을 코드로 연결
	UPROPERTY(meta = (BindWidget))
	class UH_StatBarWidget* HealthBar;

	UPROPERTY(meta = (BindWidget))
	class UH_StatBarWidget* StaminaBar;

	UPROPERTY(meta = (BindWidget))
	class UH_StatBarWidget* NoiseBar;

	UPROPERTY(meta = (BindWidget))
	class UH_WeaponWidget* WeaponUI;

	UPROPERTY(meta = (BindWidget))
	class UH_MissionWidget* MissionUI;

	UPROPERTY(meta = (BindWidgetOptional))
	class UH_TimerWidget* TimerUI;
	
	UFUNCTION(BlueprintCallable, Category = "HUD")
	APlayerCharacter* GetOwningCharacter() const;

	// 미션 업데이트 알림 표시 (블루프린트에서 애니메이션 구현)
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Mission")
	void ShowMissionNotification(const FString& Message);

};
