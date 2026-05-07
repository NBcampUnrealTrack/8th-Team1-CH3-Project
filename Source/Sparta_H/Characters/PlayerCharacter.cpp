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

	MoveSpeed = 600.f;
	SprintSpeedMultiplier = 1.5f;
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
			
			EnhancedInputComponent->BindAction(PlayerController->LeanAction, ETriggerEvent::Started, this, &APlayerCharacter::StartLean);
			EnhancedInputComponent->BindAction(PlayerController->LeanAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopLean);
			
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
	
}

void APlayerCharacter::StopHide(const FInputActionValue& value)
{
	
}

void APlayerCharacter::Roll(const FInputActionValue& value)
{
	
}

void APlayerCharacter::StartLean(const FInputActionValue& value)
{
	
}

void APlayerCharacter::StopLean(const FInputActionValue& value)
{
	
}

void APlayerCharacter::Interaction(const FInputActionValue& value)
{
	
}


