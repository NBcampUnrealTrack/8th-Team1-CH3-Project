#include "PlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "H_PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "CombatManager.h"
#include "WeaponBase.h"
#include "WeaponDataAsset.h"
#include "MissionDataAsset.h"
#include "MissionInteractableInterface.h"
#include "Kismet/GameplayStatics.h"

#include "HealthComponent.h"
#include "InteractionComponent.h"
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
	SpringArm->bUsePawnControlRotation = true; // 핵심: 마우스 따라가기
	SpringArm->bEnableCameraLag = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	// 2. 캐릭터 본체 회전 설정: 마우스 따라 몸이 돌지 않게 분리
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	

	// 3. 이동 시 회전 설정
	GetCharacterMovement()->bOrientRotationToMovement = true; // 이동 방향으로 몸 틀기
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	
	GetCharacterMovement()->bCrouchMaintainsBaseLocation = true;

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
	
	//그림자 제거
	if (GetMesh())
	{
		GetMesh()->SetCastShadow(false);
	}
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

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
	
	//카메라 회전 각도 설정
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->PlayerCameraManager)
		{
			// 위로 볼 수 있는 최대 각도 (예: 60도)
			PC->PlayerCameraManager->ViewPitchMax = 60.0f;
			// 아래로 볼 수 있는 최소 각도 (예: -60도)
			PC->PlayerCameraManager->ViewPitchMin = -45.0f;
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

	// 반동 회복 로직
	if (RecoilPitchAccum > KINDA_SMALL_NUMBER)
	{
		const float RecoverPitch = FMath::Min(RecoilPitchAccum, RecoilRecoverySpeed * DeltaTime);
		AddControllerPitchInput(RecoverPitch);
		RecoilPitchAccum -= RecoverPitch;
	}

	// 스테미나 고갈 시 달리기 멈추는 로직
	if (StaminaComponent && !StaminaComponent->CanSprint())
	{
		StopRun(FInputActionValue()); // 스태미나 고갈 시 강제 정지
	}
	
	if (!GetCharacterMovement()->IsCrouching())
	{
		SpringArm->bEnableCameraLag = false;
	}
	
	if (SpringArm)
	{
		float TargetY = LeanAmount * -MaxLeanOffset;
		
		FVector CurrentRelativeLoc = SpringArm->GetRelativeLocation();
		
		CurrentRelativeLoc.Y = FMath::FInterpTo(
			CurrentRelativeLoc.Y,
			TargetY,
			DeltaTime,
			LeanSpeed
		);
		
		SpringArm->SetRelativeLocation(CurrentRelativeLoc);
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
	}
}

void APlayerCharacter::StartJump(const FInputActionValue& value)
{
	if (value.Get<bool>())
	{
		Jump();
		
		if (JumpSound)
		{
			// 캐릭터 위치에서 점프 소리 재생
			UGameplayStatics::PlaySoundAtLocation(this, JumpSound, GetActorLocation());
		}
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
		
		if (RunSound)
		{
			// 캐릭터 위치에서 점프 소리 재생
			UGameplayStatics::PlaySoundAtLocation(this, RunSound, GetActorLocation());
		}
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
		if (SpringArm)
		{
			SpringArm->bEnableCameraLag = true;
			SpringArm->CameraLagSpeed = 12.0f; 
		}
		Crouch(); // 엔진 내장 함수 루트 컴포넌트를 아래로 내려줌
	}
}

void APlayerCharacter::StopHide(const FInputActionValue& value)
{
	UnCrouch();
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
		
		if (RollSound)
		{
			// 캐릭터 위치에서 점프 소리 재생
			UGameplayStatics::PlaySoundAtLocation(this, RollSound, GetActorLocation());
		}

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
	LeanAmount = -1.0f;
}

void APlayerCharacter::StartLeanLeft(const FInputActionValue& value)
{
	LeanAmount = 1.0f;
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

		AWeaponBase* Weapon = World->SpawnActor<AWeaponBase>(WeaponBaseClass, SpawnParams);
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

	// 투척 무기는 Started/Completed 로 처리 — Triggered 경로에서는 무시
	if (UWeaponDataAsset* Data = CurrentWeapon->GetWeaponData())
	{
		if (Data->FireMode == EWeaponFireMode::Throwable)
		{
			return;
		}
	}

	CurrentWeapon->Fire();

	// 사격 시 즉시 최대 소음 발생
	if (NoiseComponent)
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

	AWeaponBase* Spawned = World->SpawnActor<AWeaponBase>(WeaponBaseClass, SpawnParams);
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

void APlayerCharacter::ApplyRecoil(const FRecoilData& Recoil)
{
	// 음수 pitch = 카메라 위로(pitch축 기준)
	AddControllerPitchInput(-Recoil.VerticalRecoil);
	AddControllerYawInput(FMath::RandRange(-Recoil.HorizontalRecoil, Recoil.HorizontalRecoil));

	RecoilPitchAccum += Recoil.VerticalRecoil;
	RecoilRecoverySpeed = Recoil.RecoverySpeed;
}

void APlayerCharacter::NotifyEnemyKilled()
{
	// 적 처치 시 호출되는 함수.
	UE_LOG(LogTemp, Log, TEXT("Enemy Killed!"));
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
	UE_LOG(LogTemp, Error, TEXT("Mission Failed!"));
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

	// 기존 타이머 초기화
	GetWorldTimerManager().ClearTimer(MissionTimerHandle);

	if (CurrentMissionData->MissionGoals.IsValidIndex(CurrentMissionIndex))
	{
		const FMissionGoal& CurrentGoal = CurrentMissionData->MissionGoals[CurrentMissionIndex];

		// 시간 제한 설정
		if (CurrentGoal.TimeLimit > 0.0f)
		{
			GetWorldTimerManager().SetTimer(
				MissionTimerHandle,
				this,
				&APlayerCharacter::FailMission,
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

void APlayerCharacter::OnDeath()
{
	if (bIsDead) return; // 이미 죽었다면 무시

	bIsDead = true;
	UE_LOG(LogTemp, Warning, TEXT("플레이어 사망: 모든 기능을 중지합니다."));

	// 1. 입력 기능 마비
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		DisableInput(PC);
	}

	// 실패 UI 재생, 메시지 = 플레이어 사망
	
}
