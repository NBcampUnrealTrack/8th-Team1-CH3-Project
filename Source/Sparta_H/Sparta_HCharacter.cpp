// Copyright Epic Games, Inc. All Rights Reserved.

#include "Sparta_HCharacter.h"

#include "CombatManager.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Weapon/WeaponBase.h"
#include "Weapon/WeaponDataAsset.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ASparta_HCharacter

ASparta_HCharacter::ASparta_HCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// Create a CameraComponent
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	// BP에서 미지정 시 베이스 클래스로 폴백 (무기별 특수 로직이 없으면 그대로 사용)
	WeaponBaseClass = AWeaponBase::StaticClass();

	CombatManager = CreateDefaultSubobject<UCombatManager>(TEXT("CombatManager"));
}

//////////////////////////////////////////////////////////////////////////// Input

void ASparta_HCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 슬롯에 등록된 모든 무기를 미리 스폰해 GripPoint에 부착, 첫 번째 무기 자동 장착
	SpawnEquippedWeapons();

	if (!SpawnedWeapons.IsEmpty())
	{
		EquipWeaponByIndex(0);
	}
}

void ASparta_HCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ASparta_HCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASparta_HCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASparta_HCharacter::Look);

		// 슬롯 장착은 Started에서 Value가 0으로 들어오는 이슈 때문에 Triggered로 바인딩하고 콜백에서 raw 값으로 가드
		EnhancedInputComponent->BindAction(EquipSlotAction, ETriggerEvent::Triggered, this,
		                                   &ASparta_HCharacter::OnEquipSlotPressed);
		EnhancedInputComponent->BindAction(EquipNextWeaponAction, ETriggerEvent::Started, this,
		                                   &ASparta_HCharacter::OnEquipNextPressed);
		EnhancedInputComponent->BindAction(EquipPreviousWeaponAction, ETriggerEvent::Started, this,
		                                   &ASparta_HCharacter::OnEquipPreviousPressed);

		// 발사 — Triggered로 바인딩하면 풀오토 무기까지 매 틱 호출되며, 무기 측 FireRate 쿨다운이 발사 간격을 가드
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this,
		                                   &ASparta_HCharacter::OnFirePressed);

		// 재장전 - Started로 R 1회 입력 처리.
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this,
		                                   &ASparta_HCharacter::OnReloadPressed);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error,
		       TEXT(
			       "'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."
		       ), *GetNameSafe(this));
	}
}

void ASparta_HCharacter::SpawnEquippedWeapons()
{
	UWorld* World = GetWorld();
	if (World == nullptr || WeaponBaseClass == nullptr)
	{
		return;
	}

	SpawnedWeapons.Reserve(EquippedWeapons.Num());

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (UWeaponDataAsset* WeaponData : EquippedWeapons)
	{
		// 빈 슬롯은 nullptr로 보존해 EquippedWeapons와 인덱스 정합 유지
		if (WeaponData == nullptr)
		{
			SpawnedWeapons.Add(nullptr);
			continue;
		}

		AWeaponBase* Weapon = World->SpawnActor<AWeaponBase>(WeaponBaseClass, SpawnParams);
		if (Weapon == nullptr)
		{
			SpawnedWeapons.Add(nullptr);
			continue;
		}

		Weapon->Initialize(WeaponData);
		Weapon->AttachToComponent(Mesh1P,
		                          FAttachmentTransformRules::SnapToTargetIncludingScale,
		                          FName(TEXT("GripPoint")));
		Weapon->SetActorHiddenInGame(true);

		SpawnedWeapons.Add(Weapon);
	}
}

void ASparta_HCharacter::EquipWeaponByIndex(int32 NewWeaponIndex)
{
	if (!SpawnedWeapons.IsValidIndex(NewWeaponIndex))
	{
		return;
	}

	// 재장전/교체 중에는 무기 변경 차단
	if (CurrentWeapon != nullptr)
	{
		const EWeaponState State = CurrentWeapon->GetWeaponState();
		if (State == EWeaponState::Reloading || State == EWeaponState::Swapping)
		{
			return;
		}
	}

	// 같은 무기 재선택 시 무시 — Triggered 매 틱 호출에서 중복 장착 방지
	AWeaponBase* NewWeapon = SpawnedWeapons[NewWeaponIndex];
	if (NewWeapon == nullptr || NewWeapon == CurrentWeapon)
	{
		return;
	}

	if (CurrentWeapon != nullptr)
	{
		CurrentWeapon->SetActorHiddenInGame(true);
	}

	CurrentWeapon = NewWeapon;
	CurrentWeaponIndex = NewWeaponIndex;

	CurrentWeapon->SetWeaponState(EWeaponState::Swapping);
	CurrentWeapon->SetActorHiddenInGame(false);

	// 교체 몽타주 도입 시 이 라인을 종료 콜백으로 옮길 것
	CurrentWeapon->SetWeaponState(EWeaponState::Idle);
}

void ASparta_HCharacter::EquipNextWeapon()
{
	if (SpawnedWeapons.IsEmpty())
	{
		return;
	}

	const int32 NextWeaponIndex = (CurrentWeaponIndex + 1) % SpawnedWeapons.Num();
	EquipWeaponByIndex(NextWeaponIndex);
}

void ASparta_HCharacter::EquipPreviousWeapon()
{
	if (SpawnedWeapons.IsEmpty())
	{
		return;
	}

	// 음수 모듈로 회피용 + Num
	const int32 Num = SpawnedWeapons.Num();
	const int32 PreviousWeaponIndex = (CurrentWeaponIndex - 1 + Num) % Num;
	EquipWeaponByIndex(PreviousWeaponIndex);
}

void ASparta_HCharacter::OnEquipSlotPressed(const FInputActionValue& Value)
{
	// IMC에서 1/2/3/4 키에 Scalar Modifier 1.0/2.0/3.0/4.0 부여 → -1 해서 0-based 인덱스로
	const float RawValue = Value.Get<float>();

	// Triggered는 매 틱 호출됨 — 키가 실제 활성화된 상태(>= 1.0)에서만 처리
	if (RawValue < 1.0f)
	{
		return;
	}

	const int32 SlotIndex = FMath::FloorToInt(RawValue) - 1;
	EquipWeaponByIndex(SlotIndex);
}

void ASparta_HCharacter::OnEquipNextPressed(const FInputActionValue& /*Value*/)
{
	EquipNextWeapon();
}

void ASparta_HCharacter::OnEquipPreviousPressed(const FInputActionValue& /*Value*/)
{
	EquipPreviousWeapon();
}

void ASparta_HCharacter::OnFirePressed(const FInputActionValue& /*Value*/)
{
	if (CurrentWeapon != nullptr)
	{
		CurrentWeapon->Fire();
	}
}

void ASparta_HCharacter::OnReloadPressed(const FInputActionValue& Value)
{
	if (CurrentWeapon != nullptr)
	{
		CurrentWeapon->Reload();
	}
}

UWeaponDataAsset* ASparta_HCharacter::GetCurrentWeaponData() const
{
	return CurrentWeapon != nullptr ? CurrentWeapon->GetWeaponData() : nullptr;
}

void ASparta_HCharacter::NotifyEnemyKilled()
{
	// Modified: 적 처치 시 호출되는 함수. 필요 시 이곳에 로직 추가
}

void ASparta_HCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void ASparta_HCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}
