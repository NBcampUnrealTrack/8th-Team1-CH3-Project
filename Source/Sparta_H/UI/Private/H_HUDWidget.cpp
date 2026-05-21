#include "H_HUDWidget.h"
#include "PlayerCharacter.h"
#include "H_StatBarWidget.h"
#include "H_WeaponWidget.h"
#include "H_MissionWidget.h"
#include "H_TimerWidget.h"
#include "HealthComponent.h"
#include "StaminaComponent.h"
#include "NoiseComponent.h"

APlayerCharacter* UH_HUDWidget::GetOwningCharacter() const
{
	return Cast<APlayerCharacter>(GetOwningPlayerPawn());
}

//초기화 시 전용 델리게이트 바인딩 로직 구현
void UH_HUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	APlayerCharacter* Character = GetOwningCharacter();
	if (!Character)
	{
		return;
	}

	// 1. 각 스탯별 전용 델리게이트 바인딩 (데이터 혼선 방지)
	Character->OnHealthChangedUI.AddDynamic(this, &UH_HUDWidget::UpdateHealth);
	Character->OnStaminaChangedUI.AddDynamic(this, &UH_HUDWidget::UpdateStamina);
	Character->OnNoiseChangedUI.AddDynamic(this, &UH_HUDWidget::UpdateNoise);

	// 2. 무기 변경 바인딩
	Character->OnWeaponChanged.AddDynamic(this, &UH_HUDWidget::UpdateWeaponUI);

	// 3. 초기 값 설정
	UpdateWeaponUI(Character);
	UpdateMissionUI();

	// Modified: 게임 시작 시 현재 스탯 초기값을 UI에 즉시 반영 (게이지 0으로 시작하는 문제 해결)
	if (UHealthComponent* HealthComp = Character->GetHealthComponent())
	{
		UpdateHealth(HealthComp->GetCurrentHealth(), HealthComp->GetMaxHealth());
	}
	if (UStaminaComponent* StaminaComp = Character->GetStaminaComponent())
	{
		UpdateStamina(StaminaComp->GetCurrentStamina(), StaminaComp->GetMaxStamina());
	}
	if (UNoiseComponent* NoiseComp = Character->GetNoiseComponent())
	{
		UpdateNoise(NoiseComp->GetCurrentNoise(), NoiseComp->GetMaxNoise());
	}
}

void UH_HUDWidget::UpdateHealth(float Current, float Max)
{
	if (HealthBar)
	{
		HealthBar->UpdateStat(Current, Max);
		HealthBar->UpdateBarColor(Current, Max, EHStatType::Health);
	}
}

void UH_HUDWidget::UpdateStamina(float Current, float Max)
{
	if (StaminaBar)
	{
		StaminaBar->UpdateStat(Current, Max);
		StaminaBar->UpdateBarColor(Current, Max, EHStatType::Stamina);
	}
}

void UH_HUDWidget::UpdateNoise(float Current, float Max)
{
	if (NoiseBar)
	{
		NoiseBar->UpdateStat(Current, Max);
		NoiseBar->UpdateBarColor(Current, Max, EHStatType::Noise);
	}
}

void UH_HUDWidget::UpdateWeaponUI(APlayerCharacter* Character)
{
	if (WeaponUI && Character)
	{
		WeaponUI->UpdateFromCharacter(Character);
	}
}

void UH_HUDWidget::UpdateMissionUI()
{
	if (MissionUI)
	{
		MissionUI->UpdateFromCharacter(GetOwningCharacter());
	}
}

// Removed: NativeTick을 통한 매 프레임 업데이트 로직 제거 (성능 개선)
/*
void UH_HUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	...
}
*/
