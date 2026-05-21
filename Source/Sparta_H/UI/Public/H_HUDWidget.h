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
	// Modified: 타이머 실시간 업데이트를 위해 NativeTick 복구 (최소한의 로직만 수행)
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	// 위젯 초기화 시 델리게이트 바인딩 수행
	virtual void NativeConstruct() override;

	// 각 스탯 및 무기 변경 시 호출될 콜백들
	UFUNCTION()
	void UpdateHealth(float Current, float Max);
	UFUNCTION()
	void UpdateStamina(float Current, float Max);
	UFUNCTION()
	void UpdateNoise(float Current, float Max);
	UFUNCTION()
	void UpdateWeaponUI(APlayerCharacter* Character);
	UFUNCTION()
	void UpdateMissionUI();

	// Modified: 탄약 변경 이벤트를 처리하기 위한 핸들러 선언
	UFUNCTION()
	void HandleAmmoChanged(int32 CurrentAmmo, int32 MaxAmmo);

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

	UPROPERTY(meta = (BindWidget))
	class UH_ThrowableWidget* ThrowableUI;

	UPROPERTY(meta = (BindWidgetOptional))
	class UH_TimerWidget* TimerUI;
	
	UFUNCTION(BlueprintCallable, Category = "HUD")
	APlayerCharacter* GetOwningCharacter() const;

	// 미션 업데이트 알림 표시 (블루프린트에서 애니메이션 구현)
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Mission")
	void ShowMissionNotification(const FString& Message);

};
