#include "Sparta_HAmmoComponent.h"

USparta_HAmmoComponent::USparta_HAmmoComponent()
{
	// 탄약은 이벤트 기반으로만 갱신되므로 Tick 불필요
	PrimaryComponentTick.bCanEverTick = false;
}

void USparta_HAmmoComponent::InitializeAmmo(int32 NewMaxAmmoCount)
{
	MaxAmmoCount = NewMaxAmmoCount;
	CurrentAmmoCount = MaxAmmoCount;
}

bool USparta_HAmmoComponent::ConsumeAmmo()
{
	if (!HasAmmo())
	{
		return false;
	}

	CurrentAmmoCount--;
	return true;
}

void USparta_HAmmoComponent::ReloadAmmo()
{
	// 부분 재장전 없이 풀 채움. 부분 재장전 필요 시 여기서 분기
	CurrentAmmoCount = MaxAmmoCount;
}

bool USparta_HAmmoComponent::HasAmmo() const
{
	return CurrentAmmoCount > 0;
}

int32 USparta_HAmmoComponent::GetCurrentAmmoCount() const
{
	return CurrentAmmoCount;
}

int32 USparta_HAmmoComponent::GetMaxAmmoCount() const
{
	return MaxAmmoCount;
}
