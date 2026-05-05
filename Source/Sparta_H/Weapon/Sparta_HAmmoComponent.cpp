#include "Sparta_HAmmoComponent.h"

USparta_HAmmoComponent::USparta_HAmmoComponent()
{
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
