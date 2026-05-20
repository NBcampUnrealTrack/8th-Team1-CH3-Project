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

    // 상호작용 충돌 판정 영역(센서 박스) 설정
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

bool AMissionRope::CanInteract_Implementation(APlayerCharacter* Interactor) const 
{ 
    // Sweep 기반 레이캐스트 적중 시, 센서 오버랩 여부와 무관하게 즉각적인 상호작용 허용
    return true; 
}

void AMissionRope::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    RunningTime += DeltaTime * SwaySpeed;
    
    float SwayRoll = FMath::Sin(RunningTime) * SwayIntensity;
    float SwayPitch = FMath::Cos(RunningTime * 0.7f) * (SwayIntensity * 0.5f);
    
    // MainMesh 및 GlowMesh의 동적 회전 동기화
    if (MainMesh) MainMesh->SetRelativeRotation(FRotator(SwayPitch, 0.f, SwayRoll));
    if (GlowMesh) GlowMesh->SetRelativeRotation(FRotator::ZeroRotator); 

    // 시네마틱 상태에서 타겟 카메라의 렌즈 왜곡 및 오디오 볼륨 실시간 제어
    if (bIsEndingCinematic && CinematicViewTarget)
    {
        UCameraComponent* CamComp = CinematicViewTarget->FindComponentByClass<UCameraComponent>();
        if (CamComp)
        {
            CinematicElapsedTime += DeltaTime;
            float Alpha = FMath::Clamp(CinematicElapsedTime / HelicopterViewDuration, 0.0f, 1.0f);
            float SmoothAlpha = FMath::Pow(Alpha, 2.0f);

            // FOV 동적 확장 및 포스트 프로세스(색수차, 비네트)를 이용한 광속 비행 이펙트 구현
            CamComp->SetFieldOfView(FMath::Lerp(90.0f, 140.0f, SmoothAlpha));
            CamComp->PostProcessSettings.bOverride_SceneFringeIntensity = true;
            CamComp->PostProcessSettings.SceneFringeIntensity = FMath::Lerp(0.0f, 5.0f, SmoothAlpha);
            CamComp->PostProcessSettings.bOverride_VignetteIntensity = true;
            CamComp->PostProcessSettings.VignetteIntensity = FMath::Lerp(0.0f, 1.5f, SmoothAlpha);
            
            // 헬기 오디오 사운드 페이드 인 (볼륨 0.1 -> 1.0)
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

    // 기존 인게임 HUD 위젯을 화면에서 숨김 처리하여 연출 몰입도 확보
    if (AH_PlayerController* HPC = Cast<AH_PlayerController>(PC))
    {
        if (HPC->HUDWidgetInstance) HPC->HUDWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
    }
    
    // 캐릭터 조작 입력 원격 잠금
    Interactor->DisableInput(PC);

    // 오디오 컴포넌트 생성 및 최소 볼륨으로 재생 개시
    if (HelicopterSound)
    {
        HelicopterAudioComponent = UGameplayStatics::CreateSound2D(this, HelicopterSound);
        if (HelicopterAudioComponent) 
        { 
            HelicopterAudioComponent->SetVolumeMultiplier(0.1f); 
            HelicopterAudioComponent->Play(); 
        }
    }
    
    // 1단계: 카메라 페이드 아웃 (1초간 암전)
    if (PC->PlayerCameraManager) PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, 1.0f, FLinearColor::Black, false, true);

    // 엔진 글로벌 타임 딜레이를 통한 슬로우 모션(배속 30%) 활성화
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.3f);

    // 암전 종료 시점(1초 후)에 맞추어 2단계 연출(시점 전환) 타이머 등록
    FTimerHandle Handle;
    GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateUFunction(this, FName("Step2_ShowHelicopter"), Interactor), 1.0f, false);
}

void AMissionRope::Step2_ShowHelicopter(APlayerCharacter* Interactor)
{
    if (!Interactor) return;
    
    APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
    if (!PC) return;

    // 슬로우 모션 해제 및 시간 배속 정상화
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);

    // 지정된 외부 카메라(CinematicViewTarget)로 뷰포트 전환 및 카메라 쉐이크 적용
    if (CinematicViewTarget) PC->SetViewTargetWithBlend(CinematicViewTarget, 0.0f);
    if (HelicopterCameraShake && PC->PlayerCameraManager) PC->ClientStartCameraShake(HelicopterCameraShake);
    
    // 2단계: 카메라 페이드 인 (1초간 암전 해제)
    if (PC->PlayerCameraManager) PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, 1.0f, FLinearColor::Black, false, true);

    // Tick 내부의 렌즈 왜곡 연출 활성화
    bIsEndingCinematic = true;
    CinematicElapsedTime = 0.0f;

    // 시네마틱 종료 및 UI 호출을 위한 타이머 등록 (연출 시간 + 여유 시간 1초)
    FTimerHandle Handle;
    GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateUFunction(this, FName("Step3_FinishMission"), Interactor), HelicopterViewDuration + 1.0f, false);
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
            // 잠겨있던 플레이어 컨트롤러의 입력을 해제하여 엔딩 UI 클릭 및 포커스 정상화
            Interactor->EnableInput(PC);

            if (AH_PlayerController* HPC = Cast<AH_PlayerController>(PC))
            {
                // 클리어 타임 연산 및 누적 처치 수 확보
                float FinalClearTime = GetWorld()->GetTimeSeconds() - Interactor->MissionStartTime;
                int32 FinalKillCount = Interactor->KillCount;

                // 엔딩 클리어 전용 HUD 강제 호출
                HPC->ShowClearMenu(FinalClearTime, FinalKillCount);
            }
        }
    }
}

FString AMissionRope::GetInteractionText_Implementation() const 
{ 
    return TEXT("탈출하기 (F)"); 
}