#include "H_CrosshairWidget.h"
#include "PlayerCharacter.h"
#include "WeaponTypes.h"

void UH_CrosshairWidget::NativeConstruct()
{
	Super::NativeConstruct();

	APlayerCharacter* Character = Cast<APlayerCharacter>(GetOwningPlayerPawn());
	if (!Character && GetOwningPlayer())
	{
		Character = Cast<APlayerCharacter>(GetOwningPlayer()->GetPawn());
	}

	if (Character)
	{
		// 델리게이트 바인딩
		Character->OnCrosshairStateChanged.RemoveAll(this);
		Character->OnCrosshairStateChanged.AddDynamic(this, &UH_CrosshairWidget::HandleOnCrosshairStateChanged);
		
		// 초기 상태 반영
		UE_LOG(LogTemp, Log, TEXT("UH_CrosshairWidget: Initial state reflection - %d"), (int32)Character->CurrentCrosshairState);
		OnCrosshairStateChanged(Character->CurrentCrosshairState);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UH_CrosshairWidget: Failed to find APlayerCharacter in NativeConstruct"));
	}
}

ECrosshairState UH_CrosshairWidget::GetCurrentCrosshairState() const
{
	if (APlayerCharacter* Character = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
	{
		return Character->CurrentCrosshairState;
	}
	return ECrosshairState::Default;
}

void UH_CrosshairWidget::SetCrosshairState(ECrosshairState NewState)
{
	if (APlayerCharacter* Character = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
	{
		Character->SetCrosshairState(NewState);
	}
}

void UH_CrosshairWidget::HandleOnCrosshairStateChanged(ECrosshairState NewState)
{
	UE_LOG(LogTemp, Log, TEXT("UH_CrosshairWidget: Crosshair State Changed to %d"), (int32)NewState);
	// 블루프린트 이벤트 호출
	OnCrosshairStateChanged(NewState);
}