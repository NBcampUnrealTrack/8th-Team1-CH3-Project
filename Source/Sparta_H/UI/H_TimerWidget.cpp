#include "H_TimerWidget.h"
#include "Components/TextBlock.h"

void UH_TimerWidget::UpdateTimer(float RemainingTime)
{
	if (TimerText)
	{
		int32 Minutes = FMath::FloorToInt(RemainingTime / 60.0f);
		int32 Seconds = FMath::FloorToInt(FMath::Fmod(RemainingTime, 60.0f));

		FString TimerString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		TimerText->SetText(FText::FromString(TimerString));

		if (RemainingTime <= 60.0f)
		{
			TimerText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
		}
		else
		{
			TimerText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		}
	}
}
