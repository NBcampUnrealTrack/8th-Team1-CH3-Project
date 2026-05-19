#include "WeaponBase.h"
#include "AmmoComponent.h"
#include "WeaponDataAsset.h"
#include "PlayerCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "CombatManager.h"
#include "ThrowableActor.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	RootComponent = WeaponRoot;

	WeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMeshComponent"));
	WeaponMeshComponent->SetupAttachment(WeaponRoot);
	WeaponMeshComponent->SetOnlyOwnerSee(true);
	WeaponMeshComponent->bCastDynamicShadow = false;
	WeaponMeshComponent->CastShadow = false;
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AmmoComponent = CreateDefaultSubobject<UAmmoComponent>(TEXT("AmmoComponent"));
}

void AWeaponBase::Initialize(UWeaponDataAsset* InWeaponData)
{
	if (InWeaponData == nullptr)
	{
		return;
	}

	WeaponData = InWeaponData;

	if (WeaponData->FireMode == EWeaponFireMode::Throwable)
	{
		CurrentStock = WeaponData->MaxStockCount;
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

	// 투척 무기는 Fire 경로를 사용하지 않음 — BeginThrowCharge/ReleaseThrow 로 처리
	if (WeaponData->FireMode == EWeaponFireMode::Throwable)
	{
		return;
	}

	// 탄약 소모 — 재장전 가능 무기에 한해 가드. 근접/투척은 ConsumeAmmo 자체를 호출하지 않도록 추후 분기
	if (WeaponData->bIsReloadable)
	{
		if (AmmoComponent == nullptr || !AmmoComponent->ConsumeAmmo())
		{
			// 탄약 소진 - 자동 재장전 옵션이면 Reload 시작, 아니면 차단만 -> 자동 재장전으로 수정
			Reload();
			return;
		}
	}

	CurrentWeaponState = EWeaponState::Firing;
	bCanFire = false;

	// 캐릭터 본체 메시에 발사 몽타주 재생 — 풀바디 1인칭 구조라 ACharacter::GetMesh()를 사용
	if (APlayerCharacter* Character = Cast<APlayerCharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* CharacterMesh = Character->GetMesh())
		{
			if (UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance())
			{
				if (FireMontage1P)
				{
					AnimInstance->Montage_Play(FireMontage1P.Get(), 1.0f);
				}
			}
		}

		// 리코일 틱 - 몽타주와 같은 프레임
		Character->ApplyRecoil(WeaponData->RecoilData);
	}

	// 카메라 시점 기준 라인 트레이스 - 크로스헤어가 가리키는 정확한 지점에 명중하도록.
	// (총구 소켓 기준은 카메라와 어긋나서 정조준 시 빗나감)
	if (APlayerCharacter* Character = Cast<APlayerCharacter>(GetOwner()))
	{
		if (UCombatManager* CombatMgr = Character->GetCombatManager())
		{
			if (const APlayerController* PC = Cast<APlayerController>(Character->GetController()))
			{
				if (PC->PlayerCameraManager != nullptr)
				{
					const FVector AimStart = PC->PlayerCameraManager->GetCameraLocation();
					const FVector AimDirection = PC->PlayerCameraManager->GetCameraRotation().Vector();
					const ECombatWeaponType CombatType = ToCombatWeaponType(WeaponData->WeaponType);

					CombatMgr->OnFire(AimStart, AimDirection, CombatType, WeaponData->Damage,
					                  WeaponData->bShouldTriggerAIAggro,
					                  ImpactVFX.LoadSynchronous());
				}
			}
		}
	}

	// 총구 화염 — 무기 메시 소켓에 부착해 스폰. 무기가 Hidden 상태로 토글돼도 부모 메시 따라 자연스럽게 사라짐
	if (UNiagaraSystem* MuzzleFX = MuzzleFlashEffect.LoadSynchronous())
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			MuzzleFX,
			WeaponMeshComponent,
			MuzzleSocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true // bAutoDestroy — 1회성 이펙트라 끝나면 자동 정리
		);
	}

	// 발사 사운드 - 총구 소켓에 부착해 재생
	if (USoundBase* Sound = FireSound.LoadSynchronous())
	{
		UGameplayStatics::SpawnSoundAttached(
			Sound,
			WeaponMeshComponent,
			MuzzleSocketName);
	}

	// FireRate <= 0 이면 한 프레임만 차단 후 즉시 복귀
	const float Cooldown = FMath::Max(WeaponData->FireRate, KINDA_SMALL_NUMBER);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(FireCooldownTimerHandle, this, &AWeaponBase::OnFireCooldownEnded, Cooldown,
		                                  false);
	}
}

void AWeaponBase::Reload()
{
	// 상태 가드 - 리로딩/스왑 중이면 중복 호출 무시. Firing은 허용(발사 직후 R 자연스럽게)
	if (CurrentWeaponState == EWeaponState::Reloading || CurrentWeaponState == EWeaponState::Swapping)
	{
		return;
	}

	if (WeaponData == nullptr || !WeaponData->bIsReloadable)
	{
		return;
	}

	// 이미 가득이거나 탄약 컴포넌트 없으면 무시
	if (AmmoComponent == nullptr || AmmoComponent->IsFull())
	{
		return;
	}

	// 발사 쿨다운 진행 중이면 정리하고 재장전 시작
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireCooldownTimerHandle);
	}

	CurrentWeaponState = EWeaponState::Reloading;
	bCanFire = false;

	// 재장전 몽타주 재생 — 풀바디 1인칭 구조라 ACharacter::GetMesh()를 사용
	if (APlayerCharacter* Character = Cast<APlayerCharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* CharacterMesh = Character->GetMesh())
		{
			if (UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance())
			{
				if (UAnimMontage* Montage = ReloadMontage1P.LoadSynchronous())
				{
					AnimInstance->Montage_Play(Montage, 1.0f);
				}
			}
		}
	}

	// ReloadTime <= 0 이면 한 프레임만 차단 후 즉시 복귀
	const float Duration = FMath::Max(WeaponData->ReloadTime, KINDA_SMALL_NUMBER);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ReloadTimerHandle, this, &AWeaponBase::OnReloadCompleted
		                                  , Duration, false);
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

void AWeaponBase::OnReloadCompleted()
{
	if (AmmoComponent != nullptr)
	{
		AmmoComponent->ReloadAmmo();
	}
	bCanFire = true;

	// Reload도중 Swap이 들어왔으면 그 상태 보존 - Reloading 상태일 때만 Idel 복귀
	if (CurrentWeaponState == EWeaponState::Reloading)
	{
		CurrentWeaponState = EWeaponState::Idle;
	}
}

void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsChargingThrow || WeaponData == nullptr)
	{
		return;
	}

	const float Duration = FMath::Max(WeaponData->ChargeDuration, KINDA_SMALL_NUMBER);
	CurrentChargeAlpha = FMath::Clamp(CurrentChargeAlpha + DeltaTime / Duration, 0.0f, 1.0f);

	UpdateTrajectoryPreview();
}

bool AWeaponBase::HasUnlimitedStock() const
{
	return WeaponData != nullptr && WeaponData->MaxStockCount < 0;
}

bool AWeaponBase::CanThrow() const
{
	if (bThrowOnCooldown || WeaponData == nullptr)
	{
		return false;
	}
	if (HasUnlimitedStock())
	{
		return true;
	}
	return CurrentStock > 0;
}

void AWeaponBase::BeginThrowCharge()
{
	if (WeaponData == nullptr || WeaponData->FireMode != EWeaponFireMode::Throwable)
	{
		return;
	}
	if (bIsChargingThrow || !CanThrow())
	{
		return;
	}
	if (CurrentWeaponState == EWeaponState::Reloading || CurrentWeaponState == EWeaponState::Swapping)
	{
		return;
	}

	bIsChargingThrow = true;
	CurrentChargeAlpha = 0.0f;
}

void AWeaponBase::ReleaseThrow()
{
	if (!bIsChargingThrow)
	{
		return;
	}

	// 차징 상태는 무조건 해제. 이후 분기에서 실패해도 다음 입력 가능
	bIsChargingThrow = false;
	const float ChargedAlpha = CurrentChargeAlpha;
	CurrentChargeAlpha = 0.0f;

	if (WeaponData == nullptr || WeaponData->ThrowableClass == nullptr)
	{
		return;
	}

	APlayerCharacter* Character = Cast<APlayerCharacter>(GetOwner());
	if (Character == nullptr)
	{
		return;
	}

	const APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (PC == nullptr || PC->PlayerCameraManager == nullptr)
	{
		return;
	}

	const FVector Direction = PC->PlayerCameraManager->GetCameraRotation().Vector();
	// 캐릭터 캡슐 앞쪽에서 스폰 — 카메라 위치 그대로면 메시와 충돌 가능
	const FVector SpawnLocation = PC->PlayerCameraManager->GetCameraLocation() + Direction * 100.0f;
	const float Speed = FMath::Lerp(WeaponData->MinThrowSpeed, WeaponData->MaxThrowSpeed, ChargedAlpha);

	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.Instigator = Cast<APawn>(GetOwner());

	AThrowableActor* Throwable = GetWorld()->SpawnActor<AThrowableActor>(
		WeaponData->ThrowableClass, SpawnLocation, FRotator::ZeroRotator, Params);

	if (!IsValid(Throwable))
	{
		return;
	}

	Throwable->ThrowableType = ToCombatWeaponType(WeaponData->WeaponType);
	Throwable->Launch(Direction, Speed);

	// 스택 차감 (무한이 아닐 때만)
	if (!HasUnlimitedStock() && CurrentStock > 0)
	{
		--CurrentStock;
	}

	// 쿨다운 시작
	bThrowOnCooldown = true;
	if (UWorld* World = GetWorld())
	{
		const float Cooldown = FMath::Max(WeaponData->ThrowCooldown, KINDA_SMALL_NUMBER);
		World->GetTimerManager().SetTimer(ThrowCooldownTimerHandle, this,
		                                  &AWeaponBase::OnThrowCooldownEnded, Cooldown, false);
	}

	// 보유 수 0 도달 시 슬롯 복귀 통보 (무한 스택은 -1 이라 0 이 안 됨)
	if (CurrentStock == 0 && !HasUnlimitedStock())
	{
		OnDepleted.Broadcast();
	}
}

void AWeaponBase::CancelThrowCharge()
{
	if (!bIsChargingThrow)
	{
		return;
	}
	bIsChargingThrow = false;
	CurrentChargeAlpha = 0.0f;
}

void AWeaponBase::OnThrowCooldownEnded()
{
	bThrowOnCooldown = false;
}

void AWeaponBase::UpdateTrajectoryPreview()
{
	if (WeaponData == nullptr)
	{
		return;
	}

	const APlayerCharacter* Character = Cast<APlayerCharacter>(GetOwner());
	if (Character == nullptr)
	{
		return;
	}

	const APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (PC == nullptr || PC->PlayerCameraManager == nullptr)
	{
		return;
	}

	const FVector Direction = PC->PlayerCameraManager->GetCameraRotation().Vector();
	const FVector StartLocation = PC->PlayerCameraManager->GetCameraLocation() + Direction * 100.0f;
	const float Speed = FMath::Lerp(WeaponData->MinThrowSpeed, WeaponData->MaxThrowSpeed, CurrentChargeAlpha);

	FPredictProjectilePathParams PredictParams;
	PredictParams.StartLocation = StartLocation;
	PredictParams.LaunchVelocity = Direction * Speed;
	PredictParams.bTraceWithCollision = true;
	PredictParams.ProjectileRadius = WeaponData->TrajectoryProjectileRadius;
	PredictParams.MaxSimTime = WeaponData->TrajectorySimTime;
	PredictParams.SimFrequency = 30.0f;
	PredictParams.TraceChannel = ECC_Visibility;
	PredictParams.bTraceComplex = false;
	// 자신과 소유자(플레이어) 무시 — 시작 지점에서 자기 충돌 방지
	PredictParams.ActorsToIgnore.Add(this);
	if (AActor* OwnerActor = GetOwner())
	{
		PredictParams.ActorsToIgnore.Add(OwnerActor);
	}

	FPredictProjectilePathResult PredictResult;
	UGameplayStatics::PredictProjectilePath(this, PredictParams, PredictResult);

	// PathData 의 인접 두 점 사이를 라인으로 잇기. 다음 틱에 다시 그릴 거라 LifeTime=0
	for (int32 i = 0; i + 1 < PredictResult.PathData.Num(); ++i)
	{
		DrawDebugLine(
			GetWorld(),
			PredictResult.PathData[i].Location,
			PredictResult.PathData[i + 1].Location,
			FColor::Yellow,
			false,
			0.0f,
			0,
			2.0f);
	}
}

float AWeaponBase::GetThrowCooldownRemaining() const
{
	if (bThrowOnCooldown && ThrowCooldownTimerHandle.IsValid())
	{
		return GetWorldTimerManager().GetTimerRemaining(ThrowCooldownTimerHandle);
	}
	return 0.0f;
}

float AWeaponBase::GetThrowCooldownPercent() const
{
	if (bThrowOnCooldown && WeaponData && WeaponData->ThrowCooldown > 0.0f)
	{
		return FMath::Clamp(GetThrowCooldownRemaining() / WeaponData->ThrowCooldown, 0.0f, 1.0f);
	}
	return 0.0f;
}
