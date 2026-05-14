#include "UI/Public/H_FailWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

void UH_FailWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RestartButton)
	{
		RestartButton->OnClicked.AddDynamic(this, &UH_FailWidget::HandleOnRestartClicked);
	}

	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.AddDynamic(this, &UH_FailWidget::HandleOnMainMenuClicked);
	}
}

void UH_FailWidget::SetFailReasonText(const FText& FailReason)
{
	if (FailReasonText)
	{
		FailReasonText->SetText(FailReason);
	}
}

void UH_FailWidget::HandleOnRestartClicked()
{
	// 현재 레벨을 다시 시작 (리스폰 볼륨 시스템과 연동)
	// bIsDead = false;
	// Character->SetHealth(100.f);
}

void UH_FailWidget::HandleOnMainMenuClicked()
{
	// 메인 메뉴로 이동 (메인 메뉴 맵 이름이 "MainMenu"라고 가정)
	// UGameplayStatics::OpenLevel(this, FName("MainMenu"));
}
