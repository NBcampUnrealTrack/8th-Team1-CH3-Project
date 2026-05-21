#include "H_HUDWidget.h"
#include "PlayerCharacter.h"
#include "H_StatBarWidget.h"
#include "H_WeaponWidget.h"
#include "H_MissionWidget.h"
#include "H_TimerWidget.h"
#include "HealthComponent.h"
#include "StaminaComponent.h"
#include "NoiseComponent.h"
#include "WeaponBase.h"
#include "AmmoComponent.h"

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

		// Modified: 현재 장착된 무기의 탄약 컴포넌트에 이벤트 바인딩 (실시간 탄수 업데이트용)
		if (AWeaponBase* MainWeapon = Character->GetMainWeapon())
		{
			if (UAmmoComponent* AmmoComp = MainWeapon->GetAmmoComponent())
			{
				// 중복 바인딩 방지를 위해 먼저 제거 후 추가
				AmmoComp->OnAmmoChanged.RemoveAll(this);
				AmmoComp->OnAmmoChanged.AddDynamic(this, &UH_HUDWidget::HandleAmmoChanged);
			}
		}
	}
}

// Modified: 탄약 변경 시 호출될 핸들러 구현 (UI 갱신만 수행)
void UH_HUDWidget::HandleAmmoChanged(int32 CurrentAmmo, int32 MaxAmmo)
{
	if (WeaponUI)
	{
		WeaponUI->UpdateFromCharacter(GetOwningCharacter());
	}
}

void UH_HUDWidget::UpdateMissionUI()
{
	if (MissionUI)
	{
		MissionUI->UpdateFromCharacter(GetOwningCharacter());
	}
}

// Modified: 타이머 위젯의 실시간 카운트다운 표시를 위해 NativeTick 복구
void UH_HUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (TimerUI)
	{
		if (APlayerCharacter* Character = GetOwningCharacter())
		{
			float RemainingTime = Character->GetRemainingMissionTime();
			TimerUI->UpdateTimer(RemainingTime);
		}
	}
}

// Removed: 불필요한 전체 업데이트 로직 제거 (델리게이트 방식으로 대체됨)
/*
void UH_HUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	...
}
*/
