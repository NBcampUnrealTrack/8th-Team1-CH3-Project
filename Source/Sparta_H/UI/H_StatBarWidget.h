#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "H_StatBarWidget.generated.h"

UCLASS()
class SPARTA_H_API UH_StatBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 캐릭터 정보와 스탯 종류를 받아 스스로 업데이트
	UFUNCTION(BlueprintCallable, Category = "UI|Stat")
	void UpdateFromCharacter(class ASparta_HCharacter* Character, bool bIsHealth);

	// 수치 데이터를 받아 바와 텍스트를 업데이트하는 범용 함수
	UFUNCTION(BlueprintCallable, Category = "UI|Stat")
	void UpdateStat(float CurrentValue, float MaxValue);

	// 스탯 이름(예: HP, Stamina)을 설정하는 함수
	void SetStatLabel(const FString& NewLabel);

	// 바의 색상을 동적으로 변경하는 함수
	UFUNCTION(BlueprintCallable, Category = "UI|Stat")
	void SetBarColor(FLinearColor Color);

protected:
	UPROPERTY()
	FString StatLabel;

	UPROPERTY(meta = (BindWidget))
	class UProgressBar* StatProgressBar;

	// 라벨 전용 텍스트 (예: "HP")
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* LabelText;

	// 수치 전용 텍스트 (예: "100 / 100")
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* StatText;
};
