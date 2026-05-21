#include "Level/Public/Elevator.h"
#include "PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Level/Public/MapBGMPlayer.h"

AElevator::AElevator()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    ElevatorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ElevatorMesh"));
    ElevatorMesh->SetupAttachment(Root);

    OuterPanel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OuterPanel"));
    OuterPanel->SetupAttachment(Root);

    InnerPanel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InnerPanel"));
    InnerPanel->SetupAttachment(ElevatorMesh);

    LeftDoor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftDoor"));
    LeftDoor->SetupAttachment(ElevatorMesh);

    RightDoor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightDoor"));
    RightDoor->SetupAttachment(ElevatorMesh);
    
    OuterSensor = CreateDefaultSubobject<UBoxComponent>(TEXT("OuterSensor"));
    OuterSensor->SetupAttachment(OuterPanel); 
    OuterSensor->SetBoxExtent(FVector(100.f, 100.f, 100.f)); 
    OuterSensor->SetCollisionResponseToAllChannels(ECR_Ignore);
    OuterSensor->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    
    InnerSensor = CreateDefaultSubobject<UBoxComponent>(TEXT("InnerSensor"));
    InnerSensor->SetupAttachment(InnerPanel);
    InnerSensor->SetBoxExtent(FVector(100.f, 100.f, 100.f));
    InnerSensor->SetCollisionResponseToAllChannels(ECR_Ignore);
    InnerSensor->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AElevator::BeginPlay() 
{ 
    Super::BeginPlay(); 
    
    if (LeftDoor) LeftDoorInitialLoc = LeftDoor->GetRelativeLocation();
    if (RightDoor) RightDoorInitialLoc = RightDoor->GetRelativeLocation();

    if (OuterSensor)
    {
        OuterSensor->OnComponentBeginOverlap.AddDynamic(this, &AElevator::OnOuterOverlapBegin);
        OuterSensor->OnComponentEndOverlap.AddDynamic(this, &AElevator::OnOuterOverlapEnd);
    }

    if (InnerSensor)
    {
        InnerSensor->OnComponentBeginOverlap.AddDynamic(this, &AElevator::OnInnerOverlapBegin);
        InnerSensor->OnComponentEndOverlap.AddDynamic(this, &AElevator::OnInnerOverlapEnd);
    }

    if (OuterPanel) OuterPanel->SetOverlayMaterial(nullptr);
    if (InnerPanel) InnerPanel->SetOverlayMaterial(nullptr);
}

void AElevator::OnOuterOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor->IsA(APlayerCharacter::StaticClass()))
    {
        bIsPlayerInOuter = true;
        if (OuterPanel && OutlineOverlayMaterial) OuterPanel->SetOverlayMaterial(OutlineOverlayMaterial);

        APlayerController* PC = Cast<APlayerController>(Cast<APawn>(OtherActor)->GetController());
        if (PC)
        {
            EnableInput(PC);
            if (!bInputBound && InputComponent)
            {
                InputComponent->BindKey(EKeys::F, IE_Pressed, this, &AElevator::OnInteractKeyPressed);
                bInputBound = true;
            }
        }
    }
}

void AElevator::OnOuterOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (OtherActor && OtherActor->IsA(APlayerCharacter::StaticClass()))
    {
        bIsPlayerInOuter = false;
        if (OuterPanel) OuterPanel->SetOverlayMaterial(nullptr);
        APlayerController* PC = Cast<APlayerController>(Cast<APawn>(OtherActor)->GetController());
        if (PC) DisableInput(PC);
    }
}

void AElevator::OnInnerOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor->IsA(APlayerCharacter::StaticClass()))
    {
        bIsPlayerInInner = true;
        if (InnerPanel && OutlineOverlayMaterial) InnerPanel->SetOverlayMaterial(OutlineOverlayMaterial);

        APlayerController* PC = Cast<APlayerController>(Cast<APawn>(OtherActor)->GetController());
        if (PC)
        {
            EnableInput(PC);
            if (!bInputBound && InputComponent)
            {
                InputComponent->BindKey(EKeys::F, IE_Pressed, this, &AElevator::OnInteractKeyPressed);
                bInputBound = true;
            }
        }
    }
}

void AElevator::OnInnerOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (OtherActor && OtherActor->IsA(APlayerCharacter::StaticClass()))
    {
        bIsPlayerInInner = false;
        if (InnerPanel) InnerPanel->SetOverlayMaterial(nullptr);
        APlayerController* PC = Cast<APlayerController>(Cast<APawn>(OtherActor)->GetController());
        if (PC) DisableInput(PC);
    }
}

void AElevator::OnInteractKeyPressed()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        APlayerCharacter* Player = Cast<APlayerCharacter>(PC->GetPawn());
        if (Player) Interact_Implementation(Player);
    }
}

void AElevator::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!FMath::IsNearlyEqual(DoorAlpha, TargetDoorAlpha, 0.0001f))
    {
        DoorAlpha = FMath::FInterpTo(DoorAlpha, TargetDoorAlpha, DeltaTime, DoorSpeed);
        if (FMath::IsNearlyEqual(DoorAlpha, TargetDoorAlpha, 0.001f))
        {
            DoorAlpha = TargetDoorAlpha;
            if (DoorAlpha == 0.0f && CurrentState == EElevatorState::Closing) 
            {
                UpdateDoorPositions(); 
                ExecuteTeleport();
                return;
            }
            else if (DoorAlpha == 1.0f) CurrentState = EElevatorState::Open;
            else CurrentState = EElevatorState::Idle;
        }
        UpdateDoorPositions();
    }
}

void AElevator::UpdateDoorPositions()
{
    if (LeftDoor) LeftDoor->SetRelativeLocation(LeftDoorInitialLoc + FVector(DoorAlpha * DoorMoveDistance, 0.f, 0.f));
    if (RightDoor) RightDoor->SetRelativeLocation(RightDoorInitialLoc - FVector(DoorAlpha * DoorMoveDistance, 0.f, 0.f));
}

void AElevator::Interact_Implementation(APlayerCharacter* Interactor)
{
    if (!Interactor || CurrentState == EElevatorState::Teleporting) return;

    if (CurrentState == EElevatorState::Idle && bIsPlayerInOuter)
    {
        TargetDoorAlpha = 1.0f;
        CurrentState = EElevatorState::Opening;
        if (DoorOpenSound) UGameplayStatics::PlaySoundAtLocation(this, DoorOpenSound, GetActorLocation()); // 상호작용 후 문 열림 효과음 재생
    }
    else if (CurrentState == EElevatorState::Open && bIsPlayerInInner)
    {
        SavedInteractor = Interactor;
        TargetDoorAlpha = 0.0f;
        CurrentState = EElevatorState::Closing;
    }
}

void AElevator::ExecuteTeleport()
{
    if (!DestinationElevator || !SavedInteractor) return;
    CurrentState = EElevatorState::Teleporting;
    SavedInteractor->GetCharacterMovement()->DisableMovement();
    
    if (ElevatorShakeClass) GetWorld()->GetFirstPlayerController()->ClientStartCameraShake(ElevatorShakeClass);
    if (TeleportSound) UGameplayStatics::PlaySoundAtLocation(this, TeleportSound, GetActorLocation(), FRotator::ZeroRotator, TeleportVolume);

    float ZOffset = DestinationElevator->GetActorLocation().Z - GetActorLocation().Z;
    SavedInteractor->SetActorLocation(SavedInteractor->GetActorLocation() + FVector(0.f, 0.f, ZOffset));

    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &AElevator::FinishMovement, TravelDelay, false);

    TArray<AActor*> FoundPlayers;
    UGameplayStatics::GetAllActorsOfClass(this, AMapBGMPlayer::StaticClass(), FoundPlayers); // 맵의 BGM 컨트롤러를 찾아 페이드아웃 실행
    if (FoundPlayers.Num() > 0)
    {
        AMapBGMPlayer* BGMPlayer = Cast<AMapBGMPlayer>(FoundPlayers[0]);
        if (BGMPlayer) BGMPlayer->FadeOutCurrentBGM(TravelDelay); 
    }
}

void AElevator::FinishMovement()
{
    if (SavedInteractor) SavedInteractor->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    if (DestinationElevator)
    {
        if (DestinationElevator->ArrivalSound) 
            UGameplayStatics::PlaySoundAtLocation(this, DestinationElevator->ArrivalSound, DestinationElevator->GetActorLocation(), FRotator::ZeroRotator, DestinationElevator->ArrivalVolume);
        DestinationElevator->OpenDoorsAfterArrival();

        TArray<AActor*> FoundPlayers;
        UGameplayStatics::GetAllActorsOfClass(this, AMapBGMPlayer::StaticClass(), FoundPlayers); // 목적지에 도달 시 새 BGM 페이드인 실행
        if (FoundPlayers.Num() > 0)
        {
            AMapBGMPlayer* BGMPlayer = Cast<AMapBGMPlayer>(FoundPlayers[0]);
            if (BGMPlayer && DestinationElevator->StageDestinationBGM) 
            {
                BGMPlayer->FadeInNewBGM(DestinationElevator->StageDestinationBGM, DestinationElevator->BGMFadeDuration); 
            }
        }
    }
    CurrentState = EElevatorState::Idle;
}

void AElevator::OpenDoorsAfterArrival()
{
    TargetDoorAlpha = 1.0f;
    CurrentState = EElevatorState::Opening;
    if (DoorOpenSound) UGameplayStatics::PlaySoundAtLocation(this, DoorOpenSound, GetActorLocation()); // 도착 후 문 열림 효과음 재생
}

bool AElevator::CanInteract_Implementation(APlayerCharacter* Interactor) const { return true; }
FString AElevator::GetInteractionText_Implementation() const { return TEXT("상호작용 (F)"); }