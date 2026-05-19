#include "BossPrecisionWidget.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

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

void UBossPrecisionWidget::SetWarningImage(UTexture2D* Texture)
{
	if (WarningImage && Texture)
	{
		WarningImage->SetBrushFromTexture(Texture);
	}
}