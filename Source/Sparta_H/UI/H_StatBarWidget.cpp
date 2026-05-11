// Fill out your copyright notice in the Description page of Project Settings.


#include "H_StatBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "../Characters/PlayerCharacter.h"

void UH_StatBarWidget::UpdateFromCharacter(APlayerCharacter* Character, bool bIsHealth)
{
	if (!Character) return;

	if (bIsHealth)
	{
		SetStatLabel(TEXT("HP"));
		UpdateStat(Character->CurrentHealth, Character->MaxHealth);
	}
	else
	{
		SetStatLabel(TEXT("Stamina"));
		UpdateStat(Character->CurrentStamina, Character->MaxStamina);
	}
}
void UH_StatBarWidget::UpdateStat(float CurrentValue, float MaxValue)
{
	if (MaxValue <= 0.0f) return;

	float Percent = FMath::Clamp(CurrentValue / MaxValue, 0.0f, 1.0f);

	if (StatProgressBar)
	{
		StatProgressBar->SetPercent(Percent);
	}

	// Modified: 라벨 텍스트 업데이트 (예: "HP")
	if (LabelText)
	{
		LabelText->SetText(FText::FromString(StatLabel));
	}

	// Modified: 수치 텍스트 업데이트 (예: "100 / 100")
	if (StatText)
	{
		FString ValueString = FString::Printf(TEXT("%.0f / %.0f"), CurrentValue, MaxValue);
		StatText->SetText(FText::FromString(ValueString));
	}
}

void UH_StatBarWidget::SetStatLabel(const FString& NewLabel)
{
	StatLabel = NewLabel;
}

void UH_StatBarWidget::SetBarColor(FLinearColor Color)
{
	if (StatProgressBar)
	{
		StatProgressBar->SetFillColorAndOpacity(Color);
	}
}
