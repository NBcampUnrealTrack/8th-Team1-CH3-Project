#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

// Modified: 명칭 충돌 방지를 위해 EHStatType으로 변경 및 위치 이동
UENUM(BlueprintType)
enum class EHStatType : uint8
{
	Health,
	Stamina,
	Noise
};

#include "H_StatBarWidget.generated.h"

UCLASS()
class SPARTA_H_API UH_StatBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Modified: EHStatType 사용
	UFUNCTION(BlueprintCallable, Category = "UI|Stat")
	void UpdateFromCharacter(class APlayerCharacter* Character, EHStatType StatType);

	// 컴포넌트를 직접 인자로 받아 업데이트하는 기능들
	UFUNCTION(BlueprintCallable, Category = "UI|Stat")
	void UpdateFromHealthComponent(class UHealthComponent* HealthComp);

	UFUNCTION(BlueprintCallable, Category = "UI|Stat")
	void UpdateFromStaminaComponent(class UStaminaComponent* StaminaComp);

	UFUNCTION(BlueprintCallable, Category = "UI|Stat")
	void UpdateFromNoiseComponent(class UNoiseComponent* NoiseComp);

	// 수치 데이터를 받아 바와 텍스트를 업데이트하는 범용 함수
	UFUNCTION(BlueprintCallable, Category = "UI|Stat")
	void UpdateStat(float CurrentValue, float MaxValue);

	// 바의 색상을 동적으로 변경하는 함수
	UFUNCTION(BlueprintCallable, Category = "UI|Stat")
	void SetBarColor(FLinearColor Color);

	// Modified: EHStatType 사용
	void UpdateBarColor(float CurrentValue, float MaxValue, EHStatType StatType);

protected:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* StatProgressBar;

	// 수치 전용 텍스트 (예: "100 / 100")
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* StatText;
};
