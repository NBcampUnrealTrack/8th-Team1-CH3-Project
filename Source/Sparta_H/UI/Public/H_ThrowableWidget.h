#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "H_ThrowableWidget.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;
class APlayerCharacter;
class AWeaponBase;

/**
 * 오버워치 스타일의 투척 무기 UI (돌맹이/수류탄)
 */
UCLASS()
class SPARTA_H_API UH_ThrowableWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	/** 투척 무기 아이콘 **/
	UPROPERTY(meta = (BindWidget))
	UImage* ThrowableIcon;

	/** 쿨다운 오버레이 (위에서 아래로 차오르는 회색 진행도) **/
	UPROPERTY(meta = (BindWidget))
	UImage* CooldownOverlay;

	/** 수량 표시 (수류탄 전용) **/
	UPROPERTY(meta = (BindWidget))
	UTextBlock* StockCountText;
	
	/** 쿨다운 완료 시 반짝이는 효과를 위한 애니메이션 **/
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* ActivationFlashAnim;

private:
	bool bWasOnCooldown = false;
	
	UFUNCTION(BlueprintCallable, Category = "UI|Throwable")
	APlayerCharacter* GetOwningCharacter() const;

	void UpdateUI();
};
