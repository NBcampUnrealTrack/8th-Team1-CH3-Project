#include "../Framework/H_PlayerController.h"
#include "../UI/H_HUDWidget.h"
#include "Blueprint/UserWidget.h"

// Modified: ASparta_HPlayerController::BeginPlay에서 AH_PlayerController::BeginPlay로 변경
void AH_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 로컬 플레이어인 경우에만 HUD 생성
	if (IsLocalController() && HUDWidgetClass)
	{
		HUDWidgetInstance = CreateWidget<UH_HUDWidget>(this, HUDWidgetClass);
		if (HUDWidgetInstance)
		{
			HUDWidgetInstance->AddToViewport();
		}
	}
}
