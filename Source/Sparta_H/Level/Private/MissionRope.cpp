#include "Level/Public/MissionRope.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PlayerCharacter.h" 
#include "Framework/Public/H_PlayerController.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraShakeBase.h"
#include "Components/AudioComponent.h"
#include "H_HUDWidget.h"
#include "Systems/Public/MissionDataAsset.h"

AMissionRope::AMissionRope()
{
    PrimaryActorTick.bCanEverTick = true;

    if (SensorBox)
    {
        SensorBox->SetRelativeLocation(FVector(0.f, 0.f, -100.f)); 
        SensorBox->SetBoxExtent(FVector(100.f, 100.f, 150.f)); 
    }

    SwaySpeed = 2.0f;
    SwayIntensity = 5.0f;
    RunningTime = 0.0f;
}

void AMissionRope::BeginPlay() 
{ 
    Super::BeginPlay(); 
}

void AMissionRope::OnSensorOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    Super::OnSensorOverlapBegin(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

    if (OtherActor && OtherActor->IsA(APlayerCharacter::StaticClass()))
    {
        APlayerController* PC = Cast<APlayerController>(Cast<APawn>(OtherActor)->GetController());
        if (PC)
        {
            EnableInput(PC);
            if (!bInputBound && InputComponent)
            {
                InputComponent->BindKey(EKeys::F, IE_Pressed, this, &AMissionRope::OnInteractKeyPressed);
                bInputBound = true;
            }
        }
    }
}

void AMissionRope::OnSensorOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    Super::OnSensorOverlapEnd(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex);

    if (OtherActor && OtherActor->IsA(APlayerCharacter::StaticClass()))
    {
        APlayerController* PC = Cast<APlayerController>(Cast<APawn>(OtherActor)->GetController());
        if (PC)
        {
            DisableInput(PC);
        }
    }
}

void AMissionRope::OnInteractKeyPressed()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        APlayerCharacter* Player = Cast<APlayerCharacter>(PC->GetPawn());
        if (Player)
        {
            Interact_Implementation(Player);
        }
    }
}

bool AMissionRope::CanInteract_Implementation(APlayerCharacter* Interactor) const 
{ 
    return true; 
}

void AMissionRope::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    RunningTime += DeltaTime * SwaySpeed;
    
    float SwayRoll = FMath::Sin(RunningTime) * SwayIntensity;
    float SwayPitch = FMath::Cos(RunningTime * 0.7f) * (SwayIntensity * 0.5f);
    
    if (MainMesh) MainMesh->SetRelativeRotation(FRotator(SwayPitch, 0.f, SwayRoll));
    // [교정] C2065 에러를 유발하던 부모 소멸 컴포넌트(GlowMesh)의 지연 코드를 완벽히 삭제했습니다.

    if (bIsEndingCinematic && CinematicViewTarget)
    {
        UCameraComponent* CamComp = CinematicViewTarget->FindComponentByClass<UCameraComponent>();
        if (CamComp)
        {
            CinematicElapsedTime += DeltaTime;
            float Alpha = FMath::Clamp(CinematicElapsedTime / HelicopterViewDuration, 0.0f, 1.0f);
            float SmoothAlpha = FMath::Pow(Alpha, 2.0f);

            CamComp->SetFieldOfView(FMath::Lerp(90.0f, 140.0f, SmoothAlpha));
            CamComp->PostProcessSettings.bOverride_SceneFringeIntensity = true;
            CamComp->PostProcessSettings.SceneFringeIntensity = FMath::Lerp(0.0f, 5.0f, SmoothAlpha);
            CamComp->PostProcessSettings.bOverride_VignetteIntensity = true;
            CamComp->PostProcessSettings.VignetteIntensity = FMath::Lerp(0.0f, 1.5f, SmoothAlpha);
            
            if (HelicopterAudioComponent) 
                HelicopterAudioComponent->SetVolumeMultiplier(FMath::Lerp(0.1f, 1.0f, SmoothAlpha));
        }
    }
}

void AMissionRope::Interact_Implementation(APlayerCharacter* Interactor)
{
    if (!Interactor || MissionID != 4) return;

    APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
    if (!PC) return;

    if (AH_PlayerController* HPC = Cast<AH_PlayerController>(PC))
    {
        if (HPC->HUDWidgetInstance) HPC->HUDWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
    }
    
    Interactor->DisableInput(PC);

    if (HelicopterSound)
    {
        HelicopterAudioComponent = UGameplayStatics::CreateSound2D(this, HelicopterSound);
        if (HelicopterAudioComponent) 
        { 
            HelicopterAudioComponent->SetVolumeMultiplier(0.1f); 
            HelicopterAudioComponent->Play(); 
        }
    }
    
    if (PC->PlayerCameraManager) PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, 1.0f, FLinearColor::Black, false, true);

    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.3f);

    FTimerHandle Handle;
    GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateUObject(this, &AMissionRope::Step2_ShowHelicopter, Interactor), 1.0f / 0.3f, false);
}

void AMissionRope::Step2_ShowHelicopter(APlayerCharacter* Interactor)
{
    if (!Interactor) return;
    
    APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
    if (!PC) return;

    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);

    if (CinematicViewTarget) PC->SetViewTargetWithBlend(CinematicViewTarget, 0.0f);
    if (HelicopterCameraShake && PC->PlayerCameraManager) PC->ClientStartCameraShake(HelicopterCameraShake);
    
    if (PC->PlayerCameraManager) PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, 1.0f, FLinearColor::Black, false, true);

    bIsEndingCinematic = true;
    CinematicElapsedTime = 0.0f;

    FTimerHandle Handle;
    GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateUObject(this, &AMissionRope::Step3_FinishMission, Interactor), HelicopterViewDuration + 1.0f, false);
}

void AMissionRope::Step3_FinishMission(APlayerCharacter* Interactor)
{
    bIsEndingCinematic = false;
    
    if (Interactor)
    {
        APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
        if (!PC)
        {
            PC = GetWorld()->GetFirstPlayerController();
        }

        if (PC)
        {
            Interactor->EnableInput(PC);

            if (AH_PlayerController* HPC = Cast<AH_PlayerController>(PC))
            {
                float FinalClearTime = GetWorld()->GetTimeSeconds() - Interactor->MissionStartTime;
                int32 FinalKillCount = Interactor->KillCount;

                HPC->ShowClearMenu(FinalClearTime, FinalKillCount);
            }
        }
    }
}

FString AMissionRope::GetInteractionText_Implementation() const 
{ 
    return TEXT("탈출하기 (F)"); 
}