#include "H_PlayerController.h"
#include "H_HUDWidget.h"
#include "UI/Public/H_FailWidget.h"
#include "UI/Public/H_ClearWidget.h"
#include "UI/Public/H_WeaponListWidget.h"
#include "Characters/Public/PlayerCharacter.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
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

	ShowWeaponListAction = nullptr;
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

void AH_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Modified: Tab 키 무기 목록 표시/숨기기 바인딩
		if (ShowWeaponListAction)
		{
			EnhancedInputComponent->BindAction(ShowWeaponListAction, ETriggerEvent::Started, this, &AH_PlayerController::OnShowWeaponList);
			EnhancedInputComponent->BindAction(ShowWeaponListAction, ETriggerEvent::Completed, this, &AH_PlayerController::OnHideWeaponList);
		}
	}
}

void AH_PlayerController::OnShowWeaponList()
{
	ToggleWeaponList(true);
}

void AH_PlayerController::OnHideWeaponList()
{
	ToggleWeaponList(false);
}

void AH_PlayerController::ToggleWeaponList(bool bShow)
{
	// Modified: 무기 목록 위젯 토글 및 입력 모드 처리
	if (bShow)
	{
		if (!WeaponListWidgetInstance && WeaponListWidgetClass)
		{
			WeaponListWidgetInstance = CreateWidget<UH_WeaponListWidget>(this, WeaponListWidgetClass);
		}

		if (WeaponListWidgetInstance)
		{
			APlayerCharacter* PC = Cast<APlayerCharacter>(GetPawn());
			if (PC)
			{
				// 캐릭터의 소지 무기 목록을 가져와 UI 갱신
				WeaponListWidgetInstance->RefreshWeaponList(PC->GetEquippedWeapons());
			}

			WeaponListWidgetInstance->AddToViewport();
			
			// 마우스 활성화 및 게임 입력 제한
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(WeaponListWidgetInstance->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			SetInputMode(InputMode);
			bShowMouseCursor = true;
		}
	}
	else
	{
		if (WeaponListWidgetInstance)
		{
			WeaponListWidgetInstance->RemoveFromParent();
			
			// 마우스 비활성화 및 게임 전용 입력 모드 복구
			FInputModeGameOnly InputMode;
			SetInputMode(InputMode);
			bShowMouseCursor = false;
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
			FailWidget->AddToViewport();

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

void AH_PlayerController::ShowClearMenu(float ClearTime, int32 KillCount)
{
	if (ClearWidgetClass)
	{
		UH_ClearWidget* ClearWidget = CreateWidget<UH_ClearWidget>(this, ClearWidgetClass);
		if (ClearWidget)
		{
			ClearWidget->SetClearResult(ClearTime, KillCount);
			ClearWidget->AddToViewport();

			// 마우스 커서 표시 및 입력 모드 변경
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(ClearWidget->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			SetInputMode(InputMode);
			bShowMouseCursor = true;

			// 게임 일시 정지
			UGameplayStatics::SetGamePaused(GetWorld(), true);
		}
	}
}
