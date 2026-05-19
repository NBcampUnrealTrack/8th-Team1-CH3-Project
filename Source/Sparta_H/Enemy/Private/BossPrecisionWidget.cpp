#include "BossPrecisionWidget.h"

void UBossPrecisionWidget::StartWarning()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	OnWarningStarted();
}

void UBossPrecisionWidget::StopWarning()
{
	OnWarningStopped();
	SetVisibility(ESlateVisibility::Hidden);
}

void UBossPrecisionWidget::UpdateWarningProgress(float Progress)
{
	OnWarningProgressUpdated(FMath::Clamp(Progress, 0.f, 1.f));
}