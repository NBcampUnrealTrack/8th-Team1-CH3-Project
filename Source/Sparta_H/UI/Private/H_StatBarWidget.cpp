#include "H_StatBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "PlayerCharacter.h"

void UH_StatBarWidget::UpdateFromCharacter(APlayerCharacter* Character, int32 StatType)
{
	if (!Character) return;

	switch (StatType)
	{
	case 0: // Health
		UpdateStat(Character->CurrentHealth, Character->MaxHealth);
		break;
	case 1: // Stamina
		UpdateStat(Character->CurrentStamina, Character->MaxStamina);
		break;
	case 2: // Sound (Noise)
		UpdateStat(Character->CurrentNoise, Character->MaxNoise);
		break;
	default:
		break;
	}

	// 수치 업데이트 후 색상 갱신
	float Current = 0.f;
	float Max = 1.f;
	if (StatType == 0) { Current = Character->CurrentHealth; Max = Character->MaxHealth; }
	else if (StatType == 1) { Current = Character->CurrentStamina; Max = Character->MaxStamina; }
	else if (StatType == 2) { Current = Character->CurrentNoise; Max = Character->MaxNoise; }

	UpdateBarColor(Current, StatType);
}

void UH_StatBarWidget::UpdateStat(float CurrentValue, float MaxValue)
{
	if (MaxValue <= 0.0f) return;

	float Percent = FMath::Clamp(CurrentValue / MaxValue, 0.0f, 1.0f);

	if (StatProgressBar)
	{
		StatProgressBar->SetPercent(Percent);
	}

	// if (StatText)
	// {
	// 	FString ValueString = FString::Printf(TEXT("%.0f / %.0f"), CurrentValue, MaxValue);
	// 	StatText->SetText(FText::FromString(ValueString));
	// }
}

void UH_StatBarWidget::UpdateBarColor(float CurrentValue, int32 StatType)
{
	FLinearColor FinalColor = FLinearColor::White;

	if (StatType == 0) // HP
	{
		float Ratio = CurrentValue / 1.f;
		if (Ratio >= 0.7f) FinalColor = FLinearColor::Green;
		else if (Ratio >= 0.3f) FinalColor = FLinearColor::Yellow;
		else FinalColor = FLinearColor::Red;
	}
	else if (StatType == 1) // Stamina
	{
		FinalColor = FLinearColor(1.0f, 0.6f, 0.0f);
	}
	else if (StatType == 2) // Sound
	{
		FinalColor = FLinearColor::Green;
	}

	SetBarColor(FinalColor);
}

void UH_StatBarWidget::SetBarColor(FLinearColor Color)
{
	if (StatProgressBar)
	{
		StatProgressBar->SetFillColorAndOpacity(Color);
	}
}
