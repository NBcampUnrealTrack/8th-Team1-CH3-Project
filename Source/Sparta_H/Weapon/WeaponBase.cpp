#include "WeaponBase.h"

#include "AmmoComponent.h"
#include "WeaponDataAsset.h"
#include "Components/SkeletalMeshComponent.h"

AWeaponBase::AWeaponBase()
{
	// 무기 자체적으로 Tick은 불필요. 동작은 캐릭터/입력에서 이벤트 기반으로 트리거
	PrimaryActorTick.bCanEverTick = false;

	WeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMeshComponent"));
	WeaponMeshComponent->SetOnlyOwnerSee(true); // 1인칭 무기 — 본인에게만 보임
	WeaponMeshComponent->bCastDynamicShadow = false;
	WeaponMeshComponent->CastShadow = false;
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = WeaponMeshComponent;

	AmmoComponent = CreateDefaultSubobject<UAmmoComponent>(TEXT("AmmoComponent"));
}

void AWeaponBase::Initialize(UWeaponDataAsset* InWeaponData)
{
	if (InWeaponData == nullptr)
	{
		return;
	}

	WeaponData = InWeaponData;

	if (WeaponMeshComponent != nullptr)
	{
		// SoftObjectPtr 동기 로드 — 동시 다수 무기 스폰 시 비동기 로드로 전환 검토
		WeaponMeshComponent->SetSkeletalMesh(InWeaponData->WeaponMesh.LoadSynchronous());

		if (!InWeaponData->WeaponAnimationClass.IsNull())
		{
			WeaponMeshComponent->SetAnimInstanceClass(InWeaponData->WeaponAnimationClass.LoadSynchronous());
		}
	}

	if (AmmoComponent != nullptr)
	{
		AmmoComponent->InitializeAmmo(InWeaponData->MaxAmmoCount);
	}
}
