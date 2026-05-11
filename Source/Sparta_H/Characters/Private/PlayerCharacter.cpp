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

// Sets default values
APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. 스프링암 설정: 컨트롤러(마우스) 회전 사용!
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 400.f;
	SpringArm->bUsePawnControlRotation = true; // 핵심: 마우스 따라가기

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false; // 암이 돌고 있으니 이건 false로

	// 2. 캐릭터 본체 회전 설정: 마우스 따라 몸이 돌지 않게 분리
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// 3. 이동 시 회전 설정
	GetCharacterMovement()->bOrientRotationToMovement = true; // 이동 방향으로 몸 틀기
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);

	MoveSpeed = 600.f;
	SprintSpeedMultiplier = 1.5f;
	SprintSpeed = MoveSpeed * SprintSpeedMultiplier;

	CombatManager = CreateDefaultSubobject<UCombatManager>(TEXT("CombatManager"));

	// BP에서 미지정 시 베이스 클래스로 폴백 (무기별 특수 로직이 없으면 그대로 사용)
	WeaponBaseClass = AWeaponBase::StaticClass();

	MaxNoise = 100.f;
	CurrentNoise = 0.f;
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

	// 소음 수치 업데이트 (속도에 비례)
	float TargetNoise = 0.0f;
	if (GetCharacterMovement())
	{
		float VelocitySize = GetVelocity().Size();
		float MaxSpeed = 600.f;
		if (MaxSpeed > 0.0f)
		{
			TargetNoise = (VelocitySize / MaxSpeed) * 30.0f;
		}
	}
	// 부드러운 변화를 위해 보간 사용
	CurrentNoise = FMath::FInterpTo(CurrentNoise, TargetNoise, DeltaTime, 10.0f);
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
			EnhancedInputComponent->BindAction(PlayerController->MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
			
			EnhancedInputComponent->BindAction(PlayerController->LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
			
			EnhancedInputComponent->BindAction(PlayerController->JumpAction, ETriggerEvent::Started, this, &APlayerCharacter::Jump);
			EnhancedInputComponent->BindAction(PlayerController->JumpAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopJump);
			
			EnhancedInputComponent->BindAction(PlayerController->RunAction, ETriggerEvent::Started, this, &APlayerCharacter::StartRun);
			EnhancedInputComponent->BindAction(PlayerController->RunAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopRun);
			
			EnhancedInputComponent->BindAction(PlayerController->HideAction, ETriggerEvent::Started, this, &APlayerCharacter::StartHide);
			EnhancedInputComponent->BindAction(PlayerController->HideAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopHide);
			
			EnhancedInputComponent->BindAction(PlayerController->RollAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Roll);
			
			// Left (Q)
			EnhancedInputComponent->BindAction(PlayerController->LeanLeftAction, ETriggerEvent::Started, this, &APlayerCharacter::StartLeanLeft);
			EnhancedInputComponent->BindAction(PlayerController->LeanLeftAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopLean);

			// Right (E)
			EnhancedInputComponent->BindAction(PlayerController->LeanRightAction, ETriggerEvent::Started, this, &APlayerCharacter::StartLeanRight);
			EnhancedInputComponent->BindAction(PlayerController->LeanRightAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopLean);
			
			EnhancedInputComponent->BindAction(PlayerController->Interaction, ETriggerEvent::Triggered, this, &APlayerCharacter::Interaction);
			
			// 슬롯 장착은 Started에서 Value가 0으로 들어오는 이슈 때문에 Triggered로 바인딩하고 콜백에서 raw 값으로 가드
			EnhancedInputComponent->BindAction(PlayerController->EquipSlotAction, ETriggerEvent::Triggered, this,&APlayerCharacter::OnEquipSlotPressed);
			EnhancedInputComponent->BindAction(PlayerController->EquipNextWeaponAction, ETriggerEvent::Started, this,&APlayerCharacter::OnEquipNextPressed);
			EnhancedInputComponent->BindAction(PlayerController->EquipPreviousWeaponAction, ETriggerEvent::Started,this,&APlayerCharacter::OnEquipPreviousPressed);

			// 발사 — Triggered로 바인딩하면 풀오토 무기까지 매 틱 호출되며, 무기 측 FireRate 쿨다운이 발사 간격을 가드
			EnhancedInputComponent->BindAction(PlayerController->FireAction, ETriggerEvent::Triggered, this,&APlayerCharacter::OnFirePressed);

			// 재장전 — Started로 R 1회 입력 처리. 무기 상태/탄창 가드는 Reload() 내부에서
			EnhancedInputComponent->BindAction(PlayerController->ReloadAction, ETriggerEvent::Started, this,&APlayerCharacter::OnReloadPressed);
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
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void APlayerCharacter::StopRun(const FInputActionValue& value)
{
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
}

void APlayerCharacter::StartHide(const FInputActionValue& value)
{
	if (GetCharacterMovement() && !GetCharacterMovement()->IsFalling())
	{
		Crouch(); // 엔진 내장 함수 컴포넌트를 아래로 내려줌
	}
}

void APlayerCharacter::StopHide(const FInputActionValue& value)
{
	UnCrouch(); 
}

void APlayerCharacter::Roll(const FInputActionValue& value)
{
	// 이미 구르고 있거나, 몽타주가 설정되지 않았다면 리턴
	if (bIsRolling || !DiveRollMontage) return;

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
		UE_LOG(LogTemp, Log, TEXT("구르기 종료 - 이제 다시 이동 가능"));
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
	// 상호작용 라인 트레이스
	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + (Camera->GetForwardVector() * 300.0f); // 3m 사거리

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
	{
		if (AActor* HitActor = HitResult.GetActor())
		{
			if (HitActor->GetClass()->ImplementsInterface(UMissionInteractableInterface::StaticClass()))
			{
				if (IMissionInteractableInterface::Execute_CanInteract(HitActor, this))
				{
					IMissionInteractableInterface::Execute_Interact(HitActor, this);
				}
			}
		}
	}
}

void APlayerCharacter::Jump()
{
	Super::Jump();

	CurrentNoise = FMath::Clamp(CurrentNoise + 30.0f, 0.0f, MaxNoise);
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
	const int32 PreviousWeaponIndex = (CurrentWeaponIndex - 1 + Num) % Num;
	EquipWeaponByIndex(PreviousWeaponIndex);
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
	if (CurrentWeapon != nullptr)
	{
		CurrentWeapon->Fire();

		// 사격 시 즉시 최대 소음 발생
		CurrentNoise = MaxNoise;
	}
}

void APlayerCharacter::OnReloadPressed(const FInputActionValue& /*Value*/)
{
	if (CurrentWeapon != nullptr)
	{
		CurrentWeapon->Reload();
	}
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
	if (!CurrentMissionData) return;

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
	return CurrentMissionData && CurrentMissionData->MissionGoals.IsValidIndex(CurrentMissionIndex) && CurrentMissionData->MissionGoals[CurrentMissionIndex].GoalID == GoalID;
}
FName APlayerCharacter::GetCurrentObjectiveID() const
{
	return CurrentMissionData && CurrentMissionData->MissionGoals.IsValidIndex(CurrentMissionIndex) ? CurrentMissionData->MissionGoals[CurrentMissionIndex].GoalID : NAME_None;
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
	return 0.f;
}

void APlayerCharacter::UpdateMissionObjective()
{
	if (!CurrentMissionData) return;

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
