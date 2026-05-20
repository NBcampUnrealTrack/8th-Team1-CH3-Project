#include "UI/Public/H_FailWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Systems/Public/H_GameFunctionLibrary.h"

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
	// 새롭게 구현된 로드 및 리스폰 로직 호출 (체력 회복, 적 제거 포함)
	UH_GameFunctionLibrary::RequestLoadAndRespawn(this);

	// 위젯 제거 (부모 클래스의 RemoveFromParent 호출)
	RemoveFromParent();

	// 입력 모드를 게임 모드로 변경
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
	}
}

void UH_FailWidget::HandleOnMainMenuClicked()
{
	// 메인 메뉴로 이동 (메인 메뉴 맵 이름이 "MainMenu"라고 가정)
}
