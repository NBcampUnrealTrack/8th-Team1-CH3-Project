#include "AmmoComponent.h"

UAmmoComponent::UAmmoComponent()
{
	// 탄약은 이벤트 기반으로만 갱신되므로 Tick 불필요
	PrimaryComponentTick.bCanEverTick = false;
}

void UAmmoComponent::InitializeAmmo(int32 NewMaxAmmoCount)
{
	MaxAmmoCount = NewMaxAmmoCount;
	CurrentAmmoCount = MaxAmmoCount;
}

bool UAmmoComponent::ConsumeAmmo()
{
	if (!HasAmmo())
	{
		return false;
	}

	CurrentAmmoCount--;
	return true;
}

void UAmmoComponent::ReloadAmmo()
{
	// 부분 재장전 없이 풀 채움. 부분 재장전 필요 시 여기서 분기
	CurrentAmmoCount = MaxAmmoCount;
}

bool UAmmoComponent::HasAmmo() const
{
	return CurrentAmmoCount > 0;
}

int32 UAmmoComponent::GetCurrentAmmoCount() const
{
	return CurrentAmmoCount;
}

int32 UAmmoComponent::GetMaxAmmoCount() const
{
	return MaxAmmoCount;
}
