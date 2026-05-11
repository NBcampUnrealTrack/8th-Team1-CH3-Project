#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "H_StatBarWidget.generated.h"

UCLASS()
class SPARTA_H_API UH_StatBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 캐릭터 정보와 스탯 종류를 받아 스스로 업데이트 (0: Health, 1: Stamina, 2: Sound)
	UFUNCTION(BlueprintCallable, Category = "UI|Stat")
	void UpdateFromCharacter(class APlayerCharacter* Character, int32 StatType);

	// 수치 데이터를 받아 바와 텍스트를 업데이트하는 범용 함수
	UFUNCTION(BlueprintCallable, Category = "UI|Stat")
	void UpdateStat(float CurrentValue, float MaxValue);

	// 바의 색상을 동적으로 변경하는 함수
	UFUNCTION(BlueprintCallable, Category = "UI|Stat")
	void SetBarColor(FLinearColor Color);

protected:
	// 수치 및 타입에 따라 색상을 업데이트하는 내부 함수
	void UpdateBarColor(float CurrentValue, int32 StatType);

	UPROPERTY(meta = (BindWidget))
	class UProgressBar* StatProgressBar;
	
	// 수치 전용 텍스트 (예: "100 / 100")
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* StatText;
};
