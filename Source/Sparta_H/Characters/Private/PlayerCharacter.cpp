#include "PlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "H_PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "CombatManager.h"
#include "CombatFeedbackHandler.h"
#include "WeaponBase.h"
#include "WeaponDataAsset.h"
#include "MissionDataAsset.h"
#include "MissionInteractableInterface.h"
#include "Kismet/GameplayStatics.h"

#include "HealthComponent.h"
#include "InteractionComponent.h"
#include "MovieSceneSequenceID.h"
#include "NoiseComponent.h"
#include "StaminaComponent.h"
#include "VisibilityComponent.h"
#include "Components/CapsuleComponent.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));
	
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->TargetArmLength = 0.0f; // 1인칭이므로 소켓 거리는 0
	
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	// 2. 캐릭터 본체 회전 설정: 마우스 따라 몸이 돌지 않게 분리
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;


	GetMesh()->SetupAttachment(Camera);
	GetMesh()->SetCastShadow(false);
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->bCrouchMaintainsBaseLocation = true;

	// 3. 이동 시 회전 설정
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);

	MoveSpeed = 600.f;
	SprintSpeedMultiplier = 1.5f;
	SprintSpeed = MoveSpeed * SprintSpeedMultiplier;

	CombatManager = CreateDefaultSubobject<UCombatManager>(TEXT("CombatManager"));

	// BP에서 미지정 시 베이스 클래스로 폴백 (무기별 특수 로직이 없으면 그대로 사용)
	WeaponBaseClass = AWeaponBase::StaticClass();

	// 컴포넌트 추가
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	StaminaComponent = CreateDefaultSubobject<UStaminaComponent>(TEXT("StaminaComponent"));
	NoiseComponent = CreateDefaultSubobject<UNoiseComponent>(TEXT("NoiseComponent"));
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
	VisibilityComponent = CreateDefaultSubobject<UVisibilityComponent>(TEXT("VisibilityComponent"));
}

FPlayerCheckpointData APlayerCharacter::SaveCheckpoint()
{
	FPlayerCheckpointData Data;
	Data.MissionIndex = CurrentMissionIndex;
	Data.bHasRifle = bHasRifle;
	Data.Location = GetActorLocation();
	
	// 액터의 회전 대신 컨트롤러의 회전(시점)을 저장하여 상하 시점까지 정확히 기억
	if (Controller)
	{
		Data.Rotation = Controller->GetControlRotation();
	}
	else
	{
		Data.Rotation = GetActorRotation();
	}
	
	// 처치 수 및 경과 시간 저장
	Data.KillCount = KillCount;
	Data.ElapsedTime = GetWorld()->GetTimeSeconds() - MissionStartTime;
	
	return Data;
}

void APlayerCharacter::LoadCheckpoint(const FPlayerCheckpointData& CheckpointData)
{
	CurrentMissionIndex = CheckpointData.MissionIndex;
	bHasRifle = CheckpointData.bHasRifle;

	// 위치 복구
	SetActorLocation(CheckpointData.Location, false, nullptr, ETeleportType::TeleportPhysics);

	// Modified: 컨트롤러의 회전값을 설정해야 시점이 올바르게 복구됨
	if (Controller)
	{
		Controller->SetControlRotation(CheckpointData.Rotation);
	}
	else
	{
		SetActorRotation(CheckpointData.Rotation, ETeleportType::TeleportPhysics);
	}

	// Modified: 처치 수 및 시작 시간 복구 (현재 시간에서 경과 시간을 뺌)
	KillCount = CheckpointData.KillCount;
	MissionStartTime = GetWorld()->GetTimeSeconds() - CheckpointData.ElapsedTime;

	// 미션 데이터 동기화 (UI 및 내부 상태 갱신)
	UpdateMissionObjective();

	// Modified: 사망 상태 해제 및 체력 최대치로 회복
	bIsDead = false;
	if (HealthComponent)
	{
		HealthComponent->SetHealth(HealthComponent->GetMaxHealth());
	}
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 미션 시작 시간 기록
	MissionStartTime = GetWorld()->GetTimeSeconds();

	// 슬롯에 등록된 모든 무기를 미리 스폰해 본체 메시 GripPoint에 부착, 첫 번째 무기 자동 장착
	SpawnEquippedWeapons();

	if (!SpawnedWeapons.IsEmpty())
	{
		EquipWeaponByIndex(0);
	}

	// G키 투척 무기는 SpawnedWeapons 배열과 별개로 1개 스폰. Hidden 유지
	SpawnThrowableWeapon();

	// 플렝이어가 죽었을 때의 델리게이트에 함수 등록
	if (HealthComponent)
	{
		// "죽었을 때(OnDeath), 나(this)의 OnDeath 함수를 실행해줘"라고 등록
		HealthComponent->OnDeath.AddDynamic(this, &APlayerCharacter::OnDeath);
	}

	// 처치 피드백 델리게이트 등록
	if (CombatManager && CombatManager->FeedbackHandler)
	{
		CombatManager->FeedbackHandler->OnKillDelegate.AddDynamic(this, &APlayerCharacter::NotifyEnemyKilled);
	}
	
	//카메라 회전 각도 설정
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->PlayerCameraManager)
		{
			// 위로 볼 수 있는 최대 각도 
			PC->PlayerCameraManager->ViewPitchMax = 70.0f;
			// 아래로 볼 수 있는 최소 각도 
			PC->PlayerCameraManager->ViewPitchMin = -70.0f;
		}
	}

	// 미션 데이터 초기화
	CurrentMissionIndex = 0;
	UpdateMissionObjective();
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!RemainingRecoil.IsNearlyZero(0.001f))
	{
		FVector2D InsideInterp =
			FMath::Vector2DInterpTo(RemainingRecoil, FVector2D::ZeroVector, DeltaTime, RecoilSpeed);

		FVector2D RecoilToApply = RemainingRecoil - InsideInterp;
		
		AddControllerYawInput(RecoilToApply.X);
		AddControllerPitchInput(RecoilToApply.Y);
		
		RemainingRecoil = InsideInterp;
	}
	else
	{
		RemainingRecoil = FVector2D::ZeroVector;
	}
	
	// 반동 회복 로직
	if (RecoilPitchAccum > KINDA_SMALL_NUMBER)
	{
		const float RecoverPitch = FMath::Min<float>(RecoilPitchAccum, RecoilRecoverySpeed * DeltaTime);
		AddControllerPitchInput(RecoverPitch);
		RecoilPitchAccum -= RecoverPitch;
	}

	// 스테미나 고갈 시 달리기 멈추는 로직
	if (StaminaComponent && !StaminaComponent->CanSprint())
	{
		StopRun(FInputActionValue()); // 스태미나 고갈 시 강제 정지
	}
	
	// 앉기 동작시 자연스러운 카메라 보간
	if (Camera)
	{
		const float BaseCameraZ = 75.0f; 
	
		CameraZOffset = FMath::FInterpTo(CameraZOffset, 0.0f, DeltaTime, CrouchBlendSpeed);
		
		static float CurrentLeanY = 0.0f;
		float TargetY = LeanAmount * MaxLeanOffset;
		CurrentLeanY = FMath::FInterpTo(CurrentLeanY, TargetY, DeltaTime, LeanSpeed);

		// X, Y축은 칼같이 고정되고 오직 Z축(높이)만 부드럽게 움직입니다.
		Camera->SetRelativeLocation(FVector(20.0f, CurrentLeanY, BaseCameraZ + CameraZOffset));
	}
	
}

// 캐릭터 상호작용 
// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (AH_PlayerController* PlayerController = Cast<AH_PlayerController>(GetController()))
		{
			EnhancedInputComponent->BindAction(PlayerController->MoveAction, ETriggerEvent::Triggered, this,
			                                   &APlayerCharacter::Move);

			EnhancedInputComponent->BindAction(PlayerController->LookAction, ETriggerEvent::Triggered, this,
			                                   &APlayerCharacter::Look);

			EnhancedInputComponent->BindAction(PlayerController->JumpAction, ETriggerEvent::Started, this,
			                                   &APlayerCharacter::Jump);
			EnhancedInputComponent->BindAction(PlayerController->JumpAction, ETriggerEvent::Completed, this,
			                                   &APlayerCharacter::StopJump);

			EnhancedInputComponent->BindAction(PlayerController->RunAction, ETriggerEvent::Started, this,
			                                   &APlayerCharacter::StartRun);
			EnhancedInputComponent->BindAction(PlayerController->RunAction, ETriggerEvent::Completed, this,
			                                   &APlayerCharacter::StopRun);

			EnhancedInputComponent->BindAction(PlayerController->HideAction, ETriggerEvent::Started, this,
			                                   &APlayerCharacter::StartHide);
			EnhancedInputComponent->BindAction(PlayerController->HideAction, ETriggerEvent::Completed, this,
			                                   &APlayerCharacter::StopHide);

			EnhancedInputComponent->BindAction(PlayerController->RollAction, ETriggerEvent::Triggered, this,
			                                   &APlayerCharacter::Roll);

			// Left (Q)
			EnhancedInputComponent->BindAction(PlayerController->LeanLeftAction, ETriggerEvent::Started, this,
			                                   &APlayerCharacter::StartLeanLeft);
			EnhancedInputComponent->BindAction(PlayerController->LeanLeftAction, ETriggerEvent::Completed, this,
			                                   &APlayerCharacter::StopLean);

			// Right (E)
			EnhancedInputComponent->BindAction(PlayerController->LeanRightAction, ETriggerEvent::Started, this,
			                                   &APlayerCharacter::StartLeanRight);
			EnhancedInputComponent->BindAction(PlayerController->LeanRightAction, ETriggerEvent::Completed, this,
			                                   &APlayerCharacter::StopLean);

			EnhancedInputComponent->BindAction(PlayerController->Interaction, ETriggerEvent::Triggered, this,
			                                   &APlayerCharacter::Interaction);

			// 슬롯 장착은 Started에서 Value가 0으로 들어오는 이슈 때문에 Triggered로 바인딩하고 콜백에서 raw 값으로 가드
			EnhancedInputComponent->BindAction(PlayerController->EquipSlotAction, ETriggerEvent::Triggered, this,
			                                   &APlayerCharacter::OnEquipSlotPressed);
			EnhancedInputComponent->BindAction(PlayerController->EquipNextWeaponAction, ETriggerEvent::Started, this,
			                                   &APlayerCharacter::OnEquipNextPressed);
			EnhancedInputComponent->BindAction(PlayerController->EquipPreviousWeaponAction, ETriggerEvent::Started,
			                                   this, &APlayerCharacter::OnEquipPreviousPressed);

			// 발사 — Triggered로 바인딩하면 풀오토 무기까지 매 틱 호출되며, 무기 측 FireRate 쿨다운이 발사 간격을 가드
			EnhancedInputComponent->BindAction(PlayerController->FireAction, ETriggerEvent::Triggered, this,
			                                   &APlayerCharacter::OnFirePressed);

			// 투척 무기 차징/투척용 — LMB Started 에서 BeginThrowCharge, Completed 에서 ReleaseThrow
			EnhancedInputComponent->BindAction(PlayerController->FireAction, ETriggerEvent::Started, this,
			                                   &APlayerCharacter::OnFireStarted);
			EnhancedInputComponent->BindAction(PlayerController->FireAction, ETriggerEvent::Completed, this,
			                                   &APlayerCharacter::OnFireReleased);

			// 재장전 — Started로 R 1회 입력 처리. 무기 상태/탄창 가드는 Reload() 내부에서
			EnhancedInputComponent->BindAction(PlayerController->ReloadAction, ETriggerEvent::Started, this,
			                                   &APlayerCharacter::OnReloadPressed);

			// G — 투척물 장착 토글. BP 인스턴스에 아직 세팅 안 됐을 가능성에 대비해 가드
			if (PlayerController->ThrowableAction != nullptr)
			{
				EnhancedInputComponent->BindAction(PlayerController->ThrowableAction, ETriggerEvent::Started, this,
				                                   &APlayerCharacter::OnThrowableEquipPressed);
			}
		}
	}
}

void APlayerCharacter::Move(const FInputActionValue& value)
{
	const FVector2D& MoveInput = value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();

		const FRotator RightRotation(0.f, Rotation.Yaw, Rotation.Roll);
		const FVector RightDirection = FRotationMatrix(RightRotation).GetUnitAxis(EAxis::Y);


		const FRotator ForwardRotation(0.f, Rotation.Yaw, 0.f);
		const FVector ForwardDirection = FRotationMatrix(ForwardRotation).GetUnitAxis(EAxis::X);


		AddMovementInput(RightDirection, MoveInput.Y);
		AddMovementInput(ForwardDirection, MoveInput.X);
		
	}
}

void APlayerCharacter::Look(const FInputActionValue& value)
{
	const FVector2D& LookInput = value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookInput.X);
		AddControllerPitchInput(LookInput.Y);

		// Modified: 사용자가 마우스를 아래로 내려 반동을 보정하면 자동 회복량에서 차감
		if (LookInput.Y > 0.f && RecoilPitchAccum > 0.f)
		{
			RecoilPitchAccum = FMath::Max(0.f, RecoilPitchAccum - LookInput.Y);
		}
	}
}

void APlayerCharacter::StartJump(const FInputActionValue& value)
{
	if (value.Get<bool>())
	{
		Jump();
	}
}

void APlayerCharacter::StopJump(const FInputActionValue& value)
{
	if (value.Get<bool>())
	{
		StopJumping();
	}
}

void APlayerCharacter::StartRun(const FInputActionValue& value)
{
	if (StaminaComponent && StaminaComponent->CanSprint())
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
		StaminaComponent->SetSprinting(true);
	}
}

void APlayerCharacter::StopRun(const FInputActionValue& value)
{
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	if (StaminaComponent)
	{
		StaminaComponent->SetSprinting(false);
	}
}

void APlayerCharacter::StartHide(const FInputActionValue& value)
{
	if (GetCharacterMovement() && !GetCharacterMovement()->IsFalling())
	{
		CameraZOffset = 40.0f;
		
		Crouch(); // 엔진 내장 함수 루트 컴포넌트를 아래로 내려줌
	}
}

void APlayerCharacter::StopHide(const FInputActionValue& value)
{
	if (GetCharacterMovement() && GetCharacterMovement()->IsCrouching())
	{
		CameraZOffset = -40.0f;

		UnCrouch();
	}
}

void APlayerCharacter::Roll(const FInputActionValue& value)
{
	// 이미 구르고 있거나, 몽타주가 설정되지 않았다면 리턴
	if (bIsRolling || !DiveRollMontage || GetCharacterMovement()->IsFalling())
	{
		return;
	}

	// 몽타주 재생 명령
	// PlayAnimMontage는 내부적으로 AnimInstance를 찾아 Montage_Play를 호출합니다.
	float MontageLength = PlayAnimMontage(DiveRollMontage);

	if (MontageLength > 0.f)
	{
		bIsRolling = true;

		// 몽타주 종료 시점을 알기 위해 델리게이트 연결
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			// 몽타주 종료 시 호출될 콜백 함수 연결
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &APlayerCharacter::OnRollMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, DiveRollMontage);
		}
	}
}

void APlayerCharacter::OnRollMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == DiveRollMontage)
	{
		bIsRolling = false;
	}
}

void APlayerCharacter::StartLeanRight(const FInputActionValue& value)
{
	LeanAmount = 1.0f;
}

void APlayerCharacter::StartLeanLeft(const FInputActionValue& value)
{
	LeanAmount = -1.0f;
}

void APlayerCharacter::StopLean(const FInputActionValue& value)
{
	LeanAmount = 0.0f;
}

void APlayerCharacter::Interaction(const FInputActionValue& value)
{
	if (InteractionComponent)
	{
		InteractionComponent->PerformInteraction(Camera);
	}
}

void APlayerCharacter::Jump()
{
	Super::Jump();

	if (NoiseComponent)
	{
		NoiseComponent->AddNoise(30.0f);
	}
}

// 무기 관련
void APlayerCharacter::SpawnEquippedWeapons()
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

		TSubclassOf<AWeaponBase> ClassToSpawn = WeaponData->WeaponClass
			                                        ? WeaponData->WeaponClass
			                                        : WeaponBaseClass;
		AWeaponBase* Weapon = World->SpawnActor<AWeaponBase>(ClassToSpawn, SpawnParams);
		if (Weapon == nullptr)
		{
			SpawnedWeapons.Add(nullptr);
			continue;
		}

		Weapon->Initialize(WeaponData);
		Weapon->AttachToComponent(GetMesh(),
		                          FAttachmentTransformRules::SnapToTargetIncludingScale,
		                          FName(TEXT("GripPoint")));
		Weapon->SetActorHiddenInGame(true);

		SpawnedWeapons.Add(Weapon);
	}
}

void APlayerCharacter::EquipWeaponByIndex(int32 NewWeaponIndex)
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
		// 투척물 차징 중 슬롯 전환 시 차징 취소 (no-op if not charging)
		CurrentWeapon->CancelThrowCharge();
		CurrentWeapon->SetActorHiddenInGame(true);
	}

	CurrentWeapon = NewWeapon;
	CurrentWeaponIndex = NewWeaponIndex;

	CurrentWeapon->SetWeaponState(EWeaponState::Swapping);
	CurrentWeapon->SetActorHiddenInGame(false);

	// 무기 종류에 따라 크로스헤어 상태 업데이트
	if (UWeaponDataAsset* WeaponData = CurrentWeapon->GetWeaponData())
	{
		switch (WeaponData->WeaponType)
		{
		case EWeaponType::Pistol:
			SetCrosshairState(ECrosshairState::Pistol);
			break;
		case EWeaponType::Rifle:
			SetCrosshairState(ECrosshairState::Rifle);
			break;
		case EWeaponType::Knife:
			SetCrosshairState(ECrosshairState::Default);
			break;
		default:
			SetCrosshairState(ECrosshairState::Default);
			break;
		}
	}

	// 교체 몽타주 도입 시 이 라인을 종료 콜백으로 옮길 것
	CurrentWeapon->SetWeaponState(EWeaponState::Idle);
}

void APlayerCharacter::EquipNextWeapon()
{
	if (SpawnedWeapons.IsEmpty())
	{
		return;
	}

	const int32 NextWeaponIndex = (CurrentWeaponIndex + 1) % SpawnedWeapons.Num();
	EquipWeaponByIndex(NextWeaponIndex);
}

void APlayerCharacter::EquipPreviousWeapon()
{
	if (SpawnedWeapons.IsEmpty())
	{
		return;
	}

	// 음수 모듈로 회피용 + Num
	const int32 Num = SpawnedWeapons.Num();
	const int32 PrevIndex = (CurrentWeaponIndex - 1 + Num) % Num;
	EquipWeaponByIndex(PrevIndex);
}

void APlayerCharacter::OnEquipSlotPressed(const FInputActionValue& Value)
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

void APlayerCharacter::OnEquipNextPressed(const FInputActionValue& /*Value*/)
{
	EquipNextWeapon();
}

void APlayerCharacter::OnEquipPreviousPressed(const FInputActionValue& /*Value*/)
{
	EquipPreviousWeapon();
}

void APlayerCharacter::OnFirePressed(const FInputActionValue& /*Value*/)
{
	if (CurrentWeapon == nullptr)
	{
		return;
	}

	UWeaponDataAsset* Data = CurrentWeapon->GetWeaponData();

	// 투척 무기는 Started/Completed 로 처리 — Triggered 경로에서는 무시
	if (Data)
	{
		if (Data->FireMode == EWeaponFireMode::Throwable)
		{
			return;
		}
	}

	CurrentWeapon->Fire();

	// 사격 시 즉시 최대 소음 발생
	// Modified: 칼 무기는 소음을 최대로 만들지 않음
	if (NoiseComponent && Data && Data->WeaponType != EWeaponType::Knife)
	{
		NoiseComponent->SetNoiseToMax();
	}
}

void APlayerCharacter::OnReloadPressed(const FInputActionValue& /*Value*/)
{
	if (CurrentWeapon != nullptr)
	{
		CurrentWeapon->Reload();
	}
}

void APlayerCharacter::OnFireStarted(const FInputActionValue& /*Value*/)
{
	if (CurrentWeapon == nullptr)
	{
		return;
	}

	const UWeaponDataAsset* Data = CurrentWeapon->GetWeaponData();
	if (Data == nullptr || Data->FireMode != EWeaponFireMode::Throwable)
	{
		return;
	}

	CurrentWeapon->BeginThrowCharge();
}

void APlayerCharacter::OnFireReleased(const FInputActionValue& /*Value*/)
{
	if (CurrentWeapon == nullptr)
	{
		return;
	}

	const UWeaponDataAsset* Data = CurrentWeapon->GetWeaponData();
	if (Data == nullptr || Data->FireMode != EWeaponFireMode::Throwable)
	{
		return;
	}

	CurrentWeapon->ReleaseThrow();
}

void APlayerCharacter::OnThrowableEquipPressed(const FInputActionValue& /*Value*/)
{
	if (ThrowableWeapon == nullptr)
	{
		return;
	}

	// 토글 — 이미 투척물 들고 있으면 이전 슬롯으로 복귀
	if (CurrentWeapon == ThrowableWeapon)
	{
		ThrowableWeapon->CancelThrowCharge();
		EquipWeaponByIndex(PreviousWeaponIndex);
		return;
	}

	// 보유 0/쿨다운 중이면 장착 거부
	if (!ThrowableWeapon->CanThrow())
	{
		return;
	}

	// 현재 슬롯 무기 hide → 투척물 show. CurrentWeaponIndex 는 보존 (PreviousWeaponIndex 복귀용)
	if (CurrentWeapon != nullptr)
	{
		const EWeaponState State = CurrentWeapon->GetWeaponState();
		if (State == EWeaponState::Reloading || State == EWeaponState::Swapping)
		{
			return;
		}
		CurrentWeapon->SetActorHiddenInGame(true);
	}

	PreviousWeaponIndex = CurrentWeaponIndex;
	CurrentWeapon = ThrowableWeapon;
	CurrentWeapon->SetActorHiddenInGame(false);
	CurrentWeapon->SetWeaponState(EWeaponState::Idle);
}

void APlayerCharacter::HandleThrowableDepleted()
{
	// 0 도달 시 자동으로 이전 슬롯 복귀. 투척물 들고 있는 동안에만 의미 있음
	if (CurrentWeapon == ThrowableWeapon)
	{
		EquipWeaponByIndex(PreviousWeaponIndex);
	}
}

void APlayerCharacter::SpawnThrowableWeapon()
{
	if (ThrowableWeapon != nullptr || CurrentThrowableData == nullptr || WeaponBaseClass == nullptr)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TSubclassOf<AWeaponBase> ClassToSpawn = CurrentThrowableData->WeaponClass
		                                        ? CurrentThrowableData->WeaponClass
		                                        : WeaponBaseClass;
	AWeaponBase* Spawned = World->SpawnActor<AWeaponBase>(ClassToSpawn, SpawnParams);
	if (Spawned == nullptr)
	{
		return;
	}

	Spawned->Initialize(CurrentThrowableData);
	Spawned->AttachToComponent(GetMesh(),
	                           FAttachmentTransformRules::SnapToTargetIncludingScale,
	                           FName(TEXT("GripPoint")));
	Spawned->SetActorHiddenInGame(true);
	Spawned->OnDepleted.AddDynamic(this, &APlayerCharacter::HandleThrowableDepleted);

	ThrowableWeapon = Spawned;
}

void APlayerCharacter::SetActiveThrowable(UWeaponDataAsset* NewThrowableData)
{
	// 들고 있다가 미션 전환되는 경우 슬롯 먼저 복귀
	if (ThrowableWeapon != nullptr)
	{
		ThrowableWeapon->CancelThrowCharge();

		if (CurrentWeapon == ThrowableWeapon)
		{
			EquipWeaponByIndex(PreviousWeaponIndex);

			// EquipWeaponByIndex 가 실패(invalid index/빈 슬롯/상태 가드)했을 경우 첫 유효 슬롯으로 폴백.
			// 이걸 안 하면 CurrentWeapon 이 아래에서 Destroy 될 ThrowableWeapon 을 계속 가리켜 dangling 발생
			if (CurrentWeapon == ThrowableWeapon)
			{
				for (int32 i = 0; i < SpawnedWeapons.Num(); ++i)
				{
					if (SpawnedWeapons[i] != nullptr)
					{
						EquipWeaponByIndex(i);
						break;
					}
				}
			}

			// 그래도 못 바꿨으면(유효 슬롯이 하나도 없음) 명시적으로 nullptr 처리해 dangling 차단
			if (CurrentWeapon == ThrowableWeapon)
			{
				CurrentWeapon = nullptr;
			}
		}

		ThrowableWeapon->OnDepleted.RemoveAll(this);
		ThrowableWeapon->Destroy();
		ThrowableWeapon = nullptr;
	}

	CurrentThrowableData = NewThrowableData;
	SpawnThrowableWeapon();
}

UWeaponDataAsset* APlayerCharacter::GetCurrentWeaponData() const
{
	return CurrentWeapon != nullptr ? CurrentWeapon->GetWeaponData() : nullptr;
}

AWeaponBase* APlayerCharacter::GetMainWeapon() const
{
	if (SpawnedWeapons.IsValidIndex(CurrentWeaponIndex))
	{
		return SpawnedWeapons[CurrentWeaponIndex];
	}
	return nullptr;
}

void APlayerCharacter::ApplyRecoil(const FRecoilData& Recoil)
{
	// // 음수 pitch = 카메라 위로(pitch축 기준)
	// AddControllerPitchInput(-Recoil.VerticalRecoil);
	// AddControllerYawInput(FMath::RandRange(-Recoil.HorizontalRecoil, Recoil.HorizontalRecoil));
	
	float TargetPitch = -Recoil.VerticalRecoil;
	float TargetYaw = FMath::RandRange(-Recoil.HorizontalRecoil, Recoil.HorizontalRecoil);

	RemainingRecoil.X += TargetYaw;
	RemainingRecoil.Y += TargetPitch;
	
	// Modified: 반동 발생 시 누적 회복량(Pitch)을 업데이트하여 회복 로직이 작동하도록 함
	RecoilPitchAccum += Recoil.VerticalRecoil;
	RecoilSpeed = Recoil.RecoilSpeed;
	RecoilRecoverySpeed = Recoil.RecoverySpeed;
}

void APlayerCharacter::NotifyEnemyKilled()
{
	// 적 처치 시 호출되는 함수.
	UE_LOG(LogTemp, Log, TEXT("Enemy Killed!"));

	KillCount++;

	// 크로스헤어 상태를 KillConfirm으로 변경
	SetCrosshairState(ECrosshairState::KillConfirm);

	// 기존 타이머가 있다면 취소하고 새로 설정 (0.5초 후 원복)
	GetWorldTimerManager().ClearTimer(KillConfirmTimerHandle);
	GetWorldTimerManager().SetTimer(KillConfirmTimerHandle, this, &APlayerCharacter::ResetCrosshairToDefault, 0.5f,
	                                false);
}

void APlayerCharacter::SetCrosshairState(ECrosshairState NewState)
{
	if (CurrentCrosshairState != NewState)
	{
		CurrentCrosshairState = NewState;
		OnCrosshairStateChanged.Broadcast(CurrentCrosshairState);
	}
}

void APlayerCharacter::ResetCrosshairToDefault()
{
	if (CurrentWeapon == nullptr)
	{
		SetCrosshairState(ECrosshairState::Default);
		return;
	}

	// 현재 무기 종류에 맞춰 크로스헤어 상태 복구
	if (UWeaponDataAsset* WeaponData = CurrentWeapon->GetWeaponData())
	{
		switch (WeaponData->WeaponType)
		{
		case EWeaponType::Pistol:
			SetCrosshairState(ECrosshairState::Pistol);
			break;
		case EWeaponType::Rifle:
			SetCrosshairState(ECrosshairState::Rifle);
			break;
		case EWeaponType::Knife:
			SetCrosshairState(ECrosshairState::Default);
			break;
		default:
			SetCrosshairState(ECrosshairState::Default);
			break;
		}
	}
}

// 미션 관련

void APlayerCharacter::SetObjective(const FString& NewObjective)
{
	CurrentObjective = NewObjective;
	OnObjectiveChanged.Broadcast(CurrentObjective);
}

void APlayerCharacter::CompleteCurrentObjective()
{
	if (!CurrentMissionData)
	{
		return;
	}

	// 다음 목표로 이동
	CurrentMissionIndex++;

	if (CurrentMissionIndex >= CurrentMissionData->MissionGoals.Num())
	{
		// 모든 미션 최종 성공 - 타이머 해제 및 이벤트 호출
		GetWorldTimerManager().ClearTimer(MissionTimerHandle);
		OnMissionCompleted.Broadcast();
		UpdateMissionObjective();

		// 클리어 UI 표시 요청
		float ClearTime = GetWorld()->GetTimeSeconds() - MissionStartTime;
		
		APlayerController* TargetPC = Cast<APlayerController>(GetController());
		if (TargetPC == nullptr)
		{
			TargetPC = GetWorld()->GetFirstPlayerController();
		}

		if (AH_PlayerController* PC = Cast<AH_PlayerController>(TargetPC))
		{
			PC->ShowClearMenu(ClearTime, KillCount);
		}
	}
	else
	{
		UpdateMissionObjective();
	}
}

bool APlayerCharacter::IsCurrentObjective(FName GoalID) const
{
	return CurrentMissionData && CurrentMissionData->MissionGoals.IsValidIndex(CurrentMissionIndex) &&
		CurrentMissionData->MissionGoals[CurrentMissionIndex].GoalID == GoalID;
}

FName APlayerCharacter::GetCurrentObjectiveID() const
{
	return CurrentMissionData && CurrentMissionData->MissionGoals.IsValidIndex(CurrentMissionIndex)
		       ? CurrentMissionData->MissionGoals[CurrentMissionIndex].GoalID
		       : NAME_None;
}

void APlayerCharacter::FailMission()
{
	FailMissionWithReason(EMissionFailReason::Other);
}

void APlayerCharacter::FailMissionWithReason(EMissionFailReason Reason)
{
	// 타이머 정지
	GetWorldTimerManager().ClearTimer(MissionTimerHandle);

	// 실패 원인에 따른 텍스트 설정
	FText FailText;
	switch (Reason)
	{
	case EMissionFailReason::PlayerDeath:
		FailText = NSLOCTEXT("Mission", "FailReason_PlayerDeath", "플레이어가 사망했습니다.");
		break;
	case EMissionFailReason::HostageDeath:
		FailText = NSLOCTEXT("Mission", "FailReason_HostageDeath", "인질이 사망했습니다.");
		break;
	case EMissionFailReason::TimeOut:
		FailText = NSLOCTEXT("Mission", "FailReason_TimeOut", "제한 시간이 초과되었습니다.");
		break;
	case EMissionFailReason::Other:
	default:
		FailText = NSLOCTEXT("Mission", "FailReason_Other", "미션에 실패했습니다.");
		break;
	}

	// 컨트롤러에게 실패 UI 표시 요청
	// GetController()가 NULL일 수 있으므로 GetFirstPlayerController() 등을 시도
	APlayerController* TargetPC = Cast<APlayerController>(GetController());
	if (TargetPC == nullptr)
	{
		TargetPC = GetWorld()->GetFirstPlayerController();
	}

	if (AH_PlayerController* PC = Cast<AH_PlayerController>(TargetPC))
	{
		// 실패 메뉴 표시
		PC->ShowFailMenu(FailText);
	}
	// 미션 실패 처리 델리게이트 (순서를 뒤로 미룸 — UI 생성 후 호출)
	OnMissionFailed.Broadcast();
}

float APlayerCharacter::GetRemainingMissionTime() const
{
	if (GetWorldTimerManager().IsTimerActive(MissionTimerHandle))
	{
		return GetWorldTimerManager().GetTimerRemaining(MissionTimerHandle);
	}
	return 0.0f;
}

float APlayerCharacter::GetDistanceToCurrentObjective() const
{
	// Modified: FVector 간의 거리를 계산하기 위해 FVector::Dist 사용
	if (CurrentMissionData && CurrentMissionData->MissionGoals.IsValidIndex(CurrentMissionIndex))
	{
		FVector TargetLoc = CurrentMissionData->MissionGoals[CurrentMissionIndex].TargetLocation;
		if (!TargetLoc.IsZero())
		{
			return FVector::Dist(GetActorLocation(), TargetLoc) / 100.0f;
		}
	}
	return 0.f;
}

void APlayerCharacter::UpdateMissionObjective()
{
	if (!CurrentMissionData)
	{
		return;
	}

	// 미션 번호에 따른 투척 무기 교체 (미션 1~3: 돌맹이, 미션 4~: 수류탄)
	// CurrentMissionIndex는 0부터 시작하므로 3(미션 4) 이전까지는 돌맹이
	if (CurrentMissionIndex < 3)
	{
		if (RockData && CurrentThrowableData != RockData)
		{
			SetActiveThrowable(RockData);
		}
	}
	else
	{
		if (GrenadeData && CurrentThrowableData != GrenadeData)
		{
			SetActiveThrowable(GrenadeData);
		}
	}

	// 기존 타이머 초기화
	GetWorldTimerManager().ClearTimer(MissionTimerHandle);

	if (CurrentMissionData->MissionGoals.IsValidIndex(CurrentMissionIndex))
	{
		const FMissionGoal& CurrentGoal = CurrentMissionData->MissionGoals[CurrentMissionIndex];

		// 시간 제한 설정
		if (CurrentGoal.TimeLimit > 0.0f)
		{
			FTimerDelegate TimerDel;
			TimerDel.BindUFunction(this, FName("FailMissionWithReason"), EMissionFailReason::TimeOut);

			GetWorldTimerManager().SetTimer(
				MissionTimerHandle,
				TimerDel,
				CurrentGoal.TimeLimit,
				false
			);
		}

		FString DisplayText = CurrentGoal.Description;

		SetObjective(DisplayText);
	}
	else if (CurrentMissionIndex >= CurrentMissionData->MissionGoals.Num())
	{
		SetObjective(TEXT("모든 목표 달성!"));
	}
}

float APlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 대미지 발생 시 이펙트 처리를 위해 델리게이트 호출
	OnPlayerTakeDamage.Broadcast(ActualDamage);

	return ActualDamage;
}

void APlayerCharacter::OnDeath()
{
	if (bIsDead) return; // 이미 죽었다면 무시

	bIsDead = true;
	UE_LOG(LogTemp, Warning, TEXT("플레이어 사망: 모든 기능을 중지합니다."));

	// 실패 UI 재생, 메시지 = 플레이어 사망
	FailMissionWithReason(EMissionFailReason::PlayerDeath);
}
