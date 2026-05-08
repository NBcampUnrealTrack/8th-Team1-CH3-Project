// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "MyPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
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

	MoveSpeed = 300.f;
	SprintSpeedMultiplier = 2.0f;
	SprintSpeed = MoveSpeed * SprintSpeedMultiplier;
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (AMyPlayerController* PlayerController = Cast<AMyPlayerController>(GetController()))
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
	
}




