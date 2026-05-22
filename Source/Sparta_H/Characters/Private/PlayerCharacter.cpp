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
#include "Systems/Public/H_GameFunctionLibrary.h"

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

	CameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraRoot"));
	CameraRoot->SetupAttachment(RootComponent);

	// 카메라를 카메라 루트에 부착
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraRoot);

	// 1인칭 FPS 필수: 마우스 움직임에 따라 카메라 회전
	Camera->bUsePawnControlRotation = true;

	// 캐릭터 본체 회전 설정: 양옆 회전만 제어
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetMesh()->SetupAttachment(RootComponent);
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

	// 적의 ECC_Visibility 트레이스(총알)에 피격되도록 설정 — 기본 Pawn 프리셋은 Visibility를 무시함
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

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

	// 저장 시 컨트롤러의 회전을 우선하되, 컨트롤러가 없다면 액터 회전을 저장 (정규화 포함)
	if (Controller)
	{
		Data.Rotation = Controller->GetControlRotation();
	}
	else
	{
		Data.Rotation = GetActorRotation();
	}
	Data.Rotation.Normalize();

	// 처치 수 및 경과 시간 저장
	Data.KillCount = KillCount;
	Data.ElapsedTime = GetWorld()->GetTimeSeconds() - MissionStartTime;

	return Data;
}

void APlayerCharacter::LoadCheckpoint(const FPlayerCheckpointData& CheckpointData)
{
	//상태 복구 전 사망 플래그 및 물리 상태 초기화 (이동 및 회전 버그 방지)
	bIsDead = false;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->Velocity = FVector::ZeroVector;
		GetCharacterMovement()->ClearAccumulatedForces();
	}

	CurrentMissionIndex = CheckpointData.MissionIndex;
	bHasRifle = CheckpointData.bHasRifle;

	// Modified: 180도 회전 버그 수정을 위한 정밀 텔레포트 순서
	// 1. 컨트롤러 회전을 먼저 설정하여 시점의 기준을 잡음
	if (Controller)
	{
		Controller->SetControlRotation(CheckpointData.Rotation);
	}

	// 2. 위치와 회전을 동시에 복구 (컨트롤러와 동일한 회전값 사용)
	// ETeleportType::None을 사용하여 물리 엔진이 위치를 강제로 덮어쓰지 않도록 함
	SetActorLocationAndRotation(CheckpointData.Location, CheckpointData.Rotation, false, nullptr,
	                            ETeleportType::TeleportPhysics);

	// 3. 다시 한번 컨트롤러와 액터를 동기화 (레이스 컨디션 방지)
	if (Controller)
	{
		Controller->SetControlRotation(CheckpointData.Rotation);
		FaceRotation(CheckpointData.Rotation, 0.f);
	}

	KillCount = CheckpointData.KillCount;
	MissionStartTime = GetWorld()->GetTimeSeconds() - CheckpointData.ElapsedTime;

	// 미션 데이터 동기화 (UI 및 내부 상태 갱신)
	UpdateMissionObjective();

	// 체력 최대치로 회복
	if (HealthComponent)
	{
		HealthComponent->SetHealth(HealthComponent->GetMaxHealth());
	}

	// Modified: 리스폰 후 UI 스탯을 즉시 갱신하도록 강제 호출 (0으로 표시되는 문제 해결)
	HandleHealthChanged(HealthComponent ? HealthComponent->GetCurrentHealth() : 0.f,
	                    HealthComponent ? HealthComponent->GetMaxHealth() : 100.f, nullptr);
	HandleStaminaChanged(StaminaComponent ? StaminaComponent->GetCurrentStamina() : 100.f,
	                     StaminaComponent ? StaminaComponent->GetMaxStamina() : 100.f);
	HandleNoiseChanged(NoiseComponent ? NoiseComponent->GetCurrentNoise() : 0.f,
	                   NoiseComponent ? NoiseComponent->GetMaxNoise() : 100.f);

	Tags.Add(TEXT("Player"));
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 각 컴포넌트의 델리게이트에 UI 통보용 함수 연결
	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &APlayerCharacter::OnDeath);
		HealthComponent->OnHealthChanged.AddDynamic(this, &APlayerCharacter::HandleHealthChanged);
	}
	if (StaminaComponent)
	{
		StaminaComponent->OnStaminaChanged.AddDynamic(this, &APlayerCharacter::HandleStaminaChanged);
	}
	if (NoiseComponent)
	{
		NoiseComponent->OnNoiseChanged.AddDynamic(this, &APlayerCharacter::HandleNoiseChanged);
	}

	// 미션 시작 시간 기록
	MissionStartTime = GetWorld()->GetTimeSeconds();

	// 슬롯에 등록된 모든 무기를 미리 스폰해 본체 메시 GripPoint에 부착, 첫 번째 무기 자동 장착
	SpawnEquippedWeapons();

	if (!SpawnedWeapons.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("StartWeapon: bHasRifle=%s, Index=%d"),
		       bHasRifle ? TEXT("true") : TEXT("false"),
		       bHasRifle ? 0 : 1);
		const int32 StartWeaponIndex = bHasRifle ? 0 : 1;
		EquipWeaponByIndex(StartWeaponIndex);
	}

	// G키 투척 무기는 SpawnedWeapons 배열과 별개로 1개 스폰. Hidden 유지
	SpawnThrowableWeapon();

	// Removed: OnDeath 델리게이트가 위에서 이미 등록되었으므로 중복 등록 제거 (삭제됨)
	/*
	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &APlayerCharacter::OnDeath);
	}
	*/

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

	// 예약된 로드 요청이 있다면 처리
	UH_GameFunctionLibrary::HandlePendingLoad(this);
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
	if (GetMesh() && CameraRoot)
	{
		FVector HeadRelativeLoc = GetMesh()->GetSocketTransform(TEXT("head"), RTS_Actor).GetLocation();

		// 1. [기존] 사격 시 전진 Offset 보간
		static float CurrentFireForwardOffset = 0.0f;
		float TargetFireForwardOffset = bIsFiring ? 15.0f : 0.0f; 
		float FireForwardSpeed = 10.0f; 
		CurrentFireForwardOffset = FMath::FInterpTo(CurrentFireForwardOffset, TargetFireForwardOffset, DeltaTime, FireForwardSpeed);

		// 2. [수정] 기울이기(Lean) 보간 로직을 위로 올림
		static float CurrentLeanY = 0.0f;
		float TargetY = LeanAmount * MaxLeanOffset;
		CurrentLeanY = FMath::FInterpTo(CurrentLeanY, TargetY, DeltaTime, LeanSpeed);

		// 3. [기존] 상하 흔들림(Bobbing) 및 뼈대 위치 계산
		static float TimeAccumulator = 0.0f;
		TimeAccumulator += DeltaTime;

		float MovementSpeed = 3.0f;
		float MovementRange = 2.0f;

		float ZOffset = FMath::Sin(TimeAccumulator * MovementSpeed) * MovementRange;
		float TargetZ = HeadRelativeLoc.Z + ZOffset+ 10.f;

		// 4. [핵심 수정] X, Z축뿐만 아니라 Y축(기울이기)까지 포함하여 최종 목표 위치를 단 하나의 FVector로 완성합니다.
		FVector FinalCameraRootLoc = FVector(
			HeadRelativeLoc.X + 10.0f + CurrentFireForwardOffset, // X축: 기본 오프셋 + 사격 전진 값
			CurrentLeanY,                                         // Y축: [수정] 기울이기(Lean) 보간 값 직접 대입
			TargetZ                                               // Z축: 머리 뼈 위치 + 상하 흔들림
		);

		// 5. 모든 연산이 끝난 최종 위치를 CameraRoot에 딱 한 번만 적용합니다.
		CameraRoot->SetRelativeLocation(FinalCameraRootLoc);
		
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

	UpperBodyBlendWeight = 0.0f;

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
		UpperBodyBlendWeight = 1.0f;
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
	for (UWeaponDataAsset* WeaponData : EquippedWeapons)
	{
		// 빈 슬롯도 nullptr로 보존 — EquippedWeapons와 인덱스 정합 유지
		SpawnedWeapons.Add(SpawnAndAttachWeapon(WeaponData));
	}
}

AWeaponBase* APlayerCharacter::SpawnAndAttachWeapon(UWeaponDataAsset* Data)
{
	if (Data == nullptr || WeaponBaseClass == nullptr)
	{
		return nullptr;
	}
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TSubclassOf<AWeaponBase> ClassToSpawn = Data->WeaponClass ? Data->WeaponClass : WeaponBaseClass;
	AWeaponBase* Weapon = World->SpawnActor<AWeaponBase>(ClassToSpawn, SpawnParams);
	if (Weapon == nullptr)
	{
		return nullptr;
	}

	Weapon->Initialize(Data);
	Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale,
	                          Data->AttachSocketName);
	Weapon->SetActorHiddenInGame(true);

	return Weapon;
}

bool APlayerCharacter::CanEquipSlot(int32 Index) const
{
	if (!EquippedWeapons.IsValidIndex(Index))
	{
		return false;
	}
	const UWeaponDataAsset* Data = EquippedWeapons[Index];
	if (Data == nullptr)
	{
		return false;
	}

	// 라이플 미보유 상태에서는 라이플 타입 슬롯 차단
	if (Data->WeaponType == EWeaponType::Rifle && !bHasRifle)
	{
		return false;
	}

	return true;
}

void APlayerCharacter::EquipWeaponByIndex(int32 NewWeaponIndex)
{
	if (!SpawnedWeapons.IsValidIndex(NewWeaponIndex))
	{
		return;
	}

	if (!CanEquipSlot(NewWeaponIndex))
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
	// 교체 몽타주 도입 시 Swapping 상태로 전이 후 종료 콜백에서 Idle 복귀할 것
	CurrentWeapon->SetWeaponState(EWeaponState::Idle);

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

	// Modified: 무기 교체 통보 (UI 업데이트용)
	OnWeaponChanged.Broadcast(this);
}

void APlayerCharacter::EquipNextWeapon()
{
	CycleWeapon(+1);
}

// Modified: UI 성능 최적화를 위한 스탯 변경 핸들러 구현체 추가
void APlayerCharacter::HandleHealthChanged(float CurrentHealth, float MaxHealth, const UDamageType* DamageType)
{
	// UI에 전용 체력 변경 통보
	OnHealthChangedUI.Broadcast(CurrentHealth, MaxHealth);
}

void APlayerCharacter::HandleStaminaChanged(float CurrentStamina, float MaxStamina)
{
	// UI에 전용 스태미나 변경 통보
	OnStaminaChangedUI.Broadcast(CurrentStamina, MaxStamina);
}

void APlayerCharacter::HandleNoiseChanged(float CurrentNoise, float MaxNoise)
{
	// Modified: UI에 전용 소음 변경 통보가 정상적으로 전달되는지 확인
	OnNoiseChangedUI.Broadcast(CurrentNoise, MaxNoise);
}

void APlayerCharacter::EquipPreviousWeapon()
{
	CycleWeapon(-1);
}

void APlayerCharacter::CycleWeapon(int32 Direction)
{
	const int32 Num = SpawnedWeapons.Num();
	if (Num <= 0)
	{
		return;
	}

	int32 Index = CurrentWeaponIndex;
	for (int32 i = 0; i < Num; ++i)
	{
		// 음수 모듈로 회피용 + Num. CurrentWeaponIndex == INDEX_NONE 초기 상태도 안전
		Index = (Index + Direction + Num) % Num;
		if (!CanEquipSlot(Index))
		{
			continue;
		}
		if (SpawnedWeapons[Index] != nullptr)
		{
			break;
		}
	}
	EquipWeaponByIndex(Index);
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
	UpperBodyBlendWeight = 1.0f;
	
	if (bIsRolling || bIsDead)
	{
		return; // 죽었거나 구르기 상태면 사격 차단
	}

	if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
	{
		return; // 공중에 떠 있는 낙하/점프 상태라면 사격 차단
	}

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

	// 사격 시 즉시 지정된 소음 발생
	// Modified: 무기 종류에 따라 소음 수치 설정 (권총 70, 소총 100). 누적 방지를 위해 SetNoise 사용
	if (NoiseComponent && Data && Data->WeaponType != EWeaponType::Knife)
	{
		float FireNoiseAmount = (Data->WeaponType == EWeaponType::Pistol) ? 70.0f : 100.0f;
		NoiseComponent->SetNoise(FireNoiseAmount);
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
	
	if (bIsFiring)
	{
		return;
	}
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowableMontage && !bIsRolling && !bIsDead)
	{
		bIsFiring = true;
		AnimInstance->Montage_JumpToSection(TEXT("Throw"), ThrowableMontage);
		
		if (AnimInstance->Montage_GetCurrentSection(ThrowableMontage) != TEXT("Throw"))
		{
			AnimInstance->Montage_JumpToSection(TEXT("Throw"), ThrowableMontage);
		}
       
		FTimerHandle ThrowTimerHandle;
		GetWorldTimerManager().SetTimer(
		   ThrowTimerHandle, 
		   this, 
		   &APlayerCharacter::ExecuteThrow, 
		   1.0f, 
		   false
		);
	}
}

void APlayerCharacter::ExecuteThrow()
{
	CurrentWeapon->ReleaseThrow();
	bIsFiring = false;
	
	if (CurrentWeapon == ThrowableWeapon)
	{
		EquipWeaponByIndex(PreviousWeaponIndex);
	}
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
		
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && ThrowableMontage && AnimInstance->Montage_IsPlaying(ThrowableMontage))
		{
			AnimInstance->Montage_Stop(0.2f, ThrowableMontage);
		}
		
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
	
	if (ThrowableMontage && !bIsRolling && !bIsDead)
	{
		// 💡 1.0f는 재생 속도이며, TEXT("Equip")은 시작할 섹션의 이름입니다.
		PlayAnimMontage(ThrowableMontage, 1.0f, TEXT("Equip"));
	}
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
	if (ThrowableWeapon != nullptr)
	{
		return;
	}

	AWeaponBase* Spawned = SpawnAndAttachWeapon(CurrentThrowableData);
	if (Spawned == nullptr)
	{
		return;
	}

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

float APlayerCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator,
                                   AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 대미지 발생 시 이펙트 처리를 위해 델리게이트 호출
	OnPlayerTakeDamage.Broadcast(ActualDamage);

	return ActualDamage;
}

void APlayerCharacter::OnDeath()
{
	if (bIsDead)
	{
		return; // 이미 죽었다면 무시
	}

	bIsDead = true;
	UE_LOG(LogTemp, Warning, TEXT("플레이어 사망: 모든 기능을 중지합니다."));

	// 실패 UI 재생, 메시지 = 플레이어 사망
	FailMissionWithReason(EMissionFailReason::PlayerDeath);
}
