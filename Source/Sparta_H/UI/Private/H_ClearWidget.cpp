#include "UI/Public/H_ClearWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UH_ClearWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.AddDynamic(this, &UH_ClearWidget::HandleOnMainMenuClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UH_ClearWidget::HandleOnQuitClicked);
	}
}

void UH_ClearWidget::SetClearResult(float ClearTime, int32 KillCount)
{
	if (ClearTimeText)
	{
		// 시간을 분:초 형식으로 변환 (00:00 포맷팅)
		int32 Minutes = FMath::FloorToInt(ClearTime / 60.f);
		int32 Seconds = FMath::FloorToInt(ClearTime) % 60;
		
		FString TimeString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		ClearTimeText->SetText(FText::FromString(TimeString));
	}

	if (KillCountText)
	{
		KillCountText->SetText(FText::AsNumber(KillCount));
	}
}

void UH_ClearWidget::HandleOnMainMenuClicked()
{
	// 메인 메뉴로
	UGameplayStatics::OpenLevel(this, FName("MenuLevel"));
}

void UH_ClearWidget::HandleOnQuitClicked()
{
	// 게임 종료 (애플리케이션 종료)
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
