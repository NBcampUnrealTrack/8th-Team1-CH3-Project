#include "WeaponBase.h"

#include "AmmoComponent.h"
#include "WeaponDataAsset.h"
#include "../Sparta_HCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "../Systems/Public/CombatManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

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

void AWeaponBase::Fire()
{
	// 상태/쿨다운 가드 — 재장전·교체 중이거나 직전 발사 쿨다운이 안 끝났으면 무시
	if (!bCanFire || CurrentWeaponState == EWeaponState::Reloading || CurrentWeaponState == EWeaponState::Swapping)
	{
		return;
	}

	if (WeaponData == nullptr)
	{
		return;
	}

	// 탄약 소모 — 재장전 가능 무기에 한해 가드. 근접/투척은 ConsumeAmmo 자체를 호출하지 않도록 추후 분기
	if (WeaponData->bIsReloadable)
	{
		if (AmmoComponent == nullptr || !AmmoComponent->ConsumeAmmo())
		{
			// 자동 재장전은 이후 단계에서 분기. 지금은 발사만 차단
			return;
		}
	}

	CurrentWeaponState = EWeaponState::Firing;
	bCanFire = false;

	// 1인칭 팔 메시에 발사 몽타주 재생 — 무기 메시가 아닌 캐릭터의 Mesh1P 기준
	if (ASparta_HCharacter* Character = Cast<ASparta_HCharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* Mesh1P = Character->GetMesh1P())
		{
			if (UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance())
			{
				if (UAnimMontage* Montage = WeaponData->FireMontage1P.LoadSynchronous())
				{
					AnimInstance->Montage_Play(Montage, 1.0f);
				}
			}
		}
	}
	
	// 카메라 시점 기준 라인 트레이스 - 크로스헤어가 가리키는 정확한 지점에 명중하도록.
	// (총구 소켓 기준은 카메라와 어긋나서 정조준 시 빗나감)
	if (ASparta_HCharacter* Character = Cast<ASparta_HCharacter>(GetOwner()))
	{
		if (UCombatManager* CombatMgr = Character->GetCombatManager())
		{
			if (const APlayerController* PC = Cast<APlayerController>(Character->GetController()))
			{
				if (PC->PlayerCameraManager != nullptr)
				{
					const FVector AimStart     = PC->PlayerCameraManager->GetCameraLocation();
					const FVector AimDirection = PC->PlayerCameraManager->GetCameraRotation().Vector();
					const ECombatWeaponType CombatType = ToCombatWeaponType(WeaponData->WeaponType);
					CombatMgr->OnFire(AimStart, AimDirection, CombatType, WeaponData->Damage);
				}
			}
		}
	}
	
	// 총구 화염 — 무기 메시 소켓에 부착해 스폰. 무기가 Hidden 상태로 토글돼도 부모 메시 따라 자연스럽게 사라짐
	if (UNiagaraSystem* MuzzleFX = WeaponData->MuzzleFlashEffect.LoadSynchronous())
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			MuzzleFX,
			WeaponMeshComponent,
			WeaponData->MuzzleSocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true // bAutoDestroy — 1회성 이펙트라 끝나면 자동 정리
		);
	}

	// FireRate <= 0 이면 한 프레임만 차단 후 즉시 복귀
	const float Cooldown = FMath::Max(WeaponData->FireRate, KINDA_SMALL_NUMBER);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(FireCooldownTimerHandle, this, &AWeaponBase::OnFireCooldownEnded, Cooldown,
		                                  false);
	}
}

void AWeaponBase::OnFireCooldownEnded()
{
	bCanFire = true;
	// 발사 중에 재장전/교체로 전환됐으면 그 상태를 보존
	if (CurrentWeaponState == EWeaponState::Firing)
	{
		CurrentWeaponState = EWeaponState::Idle;
	}
}
