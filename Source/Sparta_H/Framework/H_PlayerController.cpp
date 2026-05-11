#include "../Framework/H_PlayerController.h"
#include "../UI/H_HUDWidget.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputSubsystems.h"

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
