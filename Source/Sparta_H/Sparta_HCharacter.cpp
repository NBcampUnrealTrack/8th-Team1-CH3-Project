// Copyright Epic Games, Inc. All Rights Reserved.

#include "Sparta_HCharacter.h"
#include "Sparta_HProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "Weapon/Sparta_HWeaponDataAsset.h"

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

	// 무기 메시는 팔 메시의 GripPoint 소켓에 부착 — 팔 애니에 자연스럽게 따라감
	WeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMeshComponent"));
	WeaponMeshComponent->SetOnlyOwnerSee(true);
	WeaponMeshComponent->SetupAttachment(Mesh1P, FName(TEXT("GripPoint")));
	WeaponMeshComponent->bCastDynamicShadow = false;
	WeaponMeshComponent->CastShadow = false;
}

//////////////////////////////////////////////////////////////////////////// Input

void ASparta_HCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 슬롯에 무기가 등록돼 있으면 첫 번째 무기를 기본 장착
	if (!EquippedWeapons.IsEmpty())
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
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error,
		       TEXT(
			       "'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."
		       ), *GetNameSafe(this));
	}
}

void ASparta_HCharacter::EquipWeaponByIndex(int32 NewWeaponIndex)
{
	if (!EquippedWeapons.IsValidIndex(NewWeaponIndex))
	{
		return;
	}

	// 재장전/교체 중에는 무기 변경 차단
	if (CurrentWeaponState == EWeaponState::Reloading || CurrentWeaponState == EWeaponState::Swapping)
	{
		return;
	}

	// 같은 무기 재선택 시 무시 — Triggered 매 틱 호출에서 중복 장착 방지
	USparta_HWeaponDataAsset* NewWeaponData = EquippedWeapons[NewWeaponIndex];
	if (NewWeaponData == nullptr || NewWeaponData == CurrentWeaponData)
	{
		return;
	}

	CurrentWeaponState = EWeaponState::Swapping;
	CurrentWeaponIndex = NewWeaponIndex;
	CurrentWeaponData = NewWeaponData;

	if (WeaponMeshComponent != nullptr)
	{
		// SoftObjectPtr는 동기 로드 — 추후 비동기 로드로 전환 검토 필요
		WeaponMeshComponent->SetSkeletalMesh(NewWeaponData->WeaponMesh.LoadSynchronous());

		if (!NewWeaponData->WeaponAnimationClass.IsNull())
		{
			WeaponMeshComponent->SetAnimInstanceClass(NewWeaponData->WeaponAnimationClass.LoadSynchronous());
		}
	}

	// 교체 애니/딜레이 들어가면 이 라인을 몽타주 종료 콜백으로 옮길 것
	CurrentWeaponState = EWeaponState::Idle;
}

void ASparta_HCharacter::EquipNextWeapon()
{
	if (EquippedWeapons.IsEmpty())
	{
		return;
	}

	const int32 NextWeaponIndex = (CurrentWeaponIndex + 1) % EquippedWeapons.Num();
	EquipWeaponByIndex(NextWeaponIndex);
}

void ASparta_HCharacter::EquipPreviousWeapon()
{
	if (EquippedWeapons.IsEmpty())
	{
		return;
	}

	// 음수 모듈로 회피용 + Num
	const int32 Num = EquippedWeapons.Num();
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
