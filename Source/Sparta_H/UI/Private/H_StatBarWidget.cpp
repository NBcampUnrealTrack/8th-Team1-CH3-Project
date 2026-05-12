#include "H_StatBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "PlayerCharacter.h"
#include "HealthComponent.h"
#include "StaminaComponent.h"
#include "NoiseComponent.h"

void UH_StatBarWidget::UpdateFromCharacter(APlayerCharacter* Character, EHStatType StatType)
{
	if (!Character) return;

	float Current = 0.f;
	float Max = 1.0f;

	// Modified: EHStatType enum 기반 switch 문 사용
	switch (StatType)
	{
	case EHStatType::Health:
		if (UHealthComponent* HealthComp = Character->GetHealthComponent())
		{
			Current = HealthComp->GetCurrentHealth();
			Max = HealthComp->GetMaxHealth();
		}
		break;
	case EHStatType::Stamina:
		if (UStaminaComponent* StaminaComp = Character->GetStaminaComponent())
		{
			Current = StaminaComp->GetCurrentStamina();
			Max = StaminaComp->GetMaxStamina();
		}
		break;
	case EHStatType::Noise:
		if (UNoiseComponent* NoiseComp = Character->GetNoiseComponent())
		{
			Current = NoiseComp->GetCurrentNoise();
			Max = NoiseComp->GetMaxNoise();
		}
		break;
	default:
		break;
	}

	UpdateStat(Current, Max);
	UpdateBarColor(Current, Max, StatType);
}

void UH_StatBarWidget::UpdateFromHealthComponent(UHealthComponent* HealthComp)
{
	if (HealthComp)
	{
		float Current = HealthComp->GetCurrentHealth();
		float Max = HealthComp->GetMaxHealth();
		UpdateStat(Current, Max);
		// Modified: EHStatType 사용
		UpdateBarColor(Current, Max, EHStatType::Health);
	}
}

void UH_StatBarWidget::UpdateFromStaminaComponent(UStaminaComponent* StaminaComp)
{
	if (StaminaComp)
	{
		float Current = StaminaComp->GetCurrentStamina();
		float Max = StaminaComp->GetMaxStamina();
		UpdateStat(Current, Max);
		// Modified: EHStatType 사용
		UpdateBarColor(Current, Max, EHStatType::Stamina);
	}
}

void UH_StatBarWidget::UpdateFromNoiseComponent(UNoiseComponent* NoiseComp)
{
	if (NoiseComp)
	{
		float Current = NoiseComp->GetCurrentNoise();
		float Max = NoiseComp->GetMaxNoise();
		UpdateStat(Current, Max);
		// Modified: EHStatType 사용
		UpdateBarColor(Current, Max, EHStatType::Noise);
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

	// if (StatText)
	// {
	// 	FString ValueString = FString::Printf(TEXT("%.0f / %.0f"), CurrentValue, MaxValue);
	// 	StatText->SetText(FText::FromString(ValueString));
	// }
}

void UH_StatBarWidget::UpdateBarColor(float CurrentValue, float MaxValue, EHStatType StatType)
{
	if (MaxValue <= 0.0f) return;
	
	FLinearColor FinalColor = FLinearColor::White;
	float Ratio = CurrentValue / MaxValue;

	// Modified: EHStatType enum 기반으로 변경
	if (StatType == EHStatType::Health)
	{
		if (Ratio >= 0.7f) FinalColor = FLinearColor::Green;
		else if (Ratio >= 0.3f) FinalColor = FLinearColor::Yellow;
		else FinalColor = FLinearColor::Red;
	}
	else if (StatType == EHStatType::Stamina)
	{
		FinalColor = FLinearColor(1.0f, 0.6f, 0.0f);
	}
	else if (StatType == EHStatType::Noise)
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
