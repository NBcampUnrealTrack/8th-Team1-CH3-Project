#include "H_PlayerController.h"
#include "H_HUDWidget.h"
#include "UI/Public/H_FailWidget.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"

AH_PlayerController::AH_PlayerController()
{
	InputMappingContext = nullptr; // 초기화 후 블루프린트에서 대입
	MoveAction = nullptr;
	LookAction = nullptr;
	JumpAction = nullptr;
	RunAction = nullptr;
	HideAction = nullptr;
	RollAction = nullptr;
	LeanRightAction = nullptr;	
	LeanLeftAction = nullptr;
	Interaction = nullptr;
	
	FireAction = nullptr;
	ReloadAction = nullptr;
	EquipSlotAction = nullptr;
	EquipNextWeaponAction = nullptr;
	EquipPreviousWeaponAction = nullptr;
}

void AH_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (InputMappingContext)
			{
				Subsystem->AddMappingContext(InputMappingContext, 0);
			}
		}
	}

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

void AH_PlayerController::ShowFailMenu(const FText& Reason)
{
	if (FailWidgetClass)
	{
		UH_FailWidget* FailWidget = CreateWidget<UH_FailWidget>(this, FailWidgetClass);
		if (FailWidget)
		{
			FailWidget->SetFailReasonText(Reason);
			FailWidget->AddToViewport(10); // HUD보다 위에 표시되도록 Z-Order 설정

			// 마우스 커서 표시 및 입력 모드 변경
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(FailWidget->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			SetInputMode(InputMode);
			bShowMouseCursor = true;

			// 게임 일시 정지
			UGameplayStatics::SetGamePaused(GetWorld(), true);
		}
	}
}
