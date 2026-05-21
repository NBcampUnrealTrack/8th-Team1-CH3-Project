// Copyright Epic Games, Inc. All Rights Reserved.

#include "H_GameMode.h"
#include "PlayerCharacter.h"
#include "H_PlayerController.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AH_GameMode::AH_GameMode()
	: Super()
{
	DefaultPawnClass = APlayerCharacter::StaticClass(); // 기본 폰 클래스 할당
	PlayerControllerClass = AH_PlayerController::StaticClass(); // 기본 컨트롤러 클래스 할당
    CurrentBGMComponent = nullptr; // 초기 컴포넌트 포인터 null 초기화
    PendingBGM = nullptr; // 대기 중인 오디오 에셋 초기화
    InitialBGM = nullptr; // 초기 BGM 에셋 초기화
}

void AH_GameMode::BeginPlay()
{
    Super::BeginPlay(); // 부모 클래스 BeginPlay 호출

    if (InitialBGM)
    {
        // 자동 재생 및 즉시 컬링 방지를 위해 CreateSound2D 호출 후 별도 페이드인 적용
        CurrentBGMComponent = UGameplayStatics::CreateSound2D(this, InitialBGM, 1.0f, 1.0f, 0.0f, nullptr, false, true);
        if (CurrentBGMComponent)
        {
            CurrentBGMComponent->FadeIn(1.0f, 1.0f); // 1초에 걸쳐 정상 볼륨으로 최초 진입 재생
        }
    }
}

void AH_GameMode::TransitionBGM(USoundBase* NewBGM, float FadeOutDuration, float DelayDuration)
{
    PendingBGM = NewBGM; // 새 오디오 에셋 포인터 저장

    if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying())
    {
        CurrentBGMComponent->FadeOut(FadeOutDuration, 0.0f); // 활성화된 기존 BGM 컴포넌트 페이드아웃 실행

        if (GetWorld())
        {
            GetWorld()->GetTimerManager().SetTimer(BGMTransitionTimerHandle, this, &AH_GameMode::PlayNextBGMDeferred, DelayDuration, false); // 설정된 지연 시간 후 재생 함수 호출 예약
        }
    }
    else
    {
        PlayNextBGMDeferred(); // 기존에 재생 중인 BGM이 없으면 즉시 새 사운드 재생
    }
}

void AH_GameMode::PlayNextBGMDeferred()
{
    if (!PendingBGM)
    {
        return; // 대기 오디오 에셋이 유효하지 않으면 반환
    }

    if (CurrentBGMComponent)
    {
        CurrentBGMComponent->Stop(); // 리소스 완전 점유 해제를 위한 정지 처리
        CurrentBGMComponent = nullptr; // 기존 포인터 명시적 초기화
    }

    // 엔진의 볼륨 0.0 즉시 강제 소멸 현상을 막기 위해 Spawn 대신 Create 컴포넌트 함수 사용
    CurrentBGMComponent = UGameplayStatics::CreateSound2D(this, PendingBGM, 1.0f, 1.0f, 0.0f, nullptr, false, true);

    if (CurrentBGMComponent)
    {
        CurrentBGMComponent->FadeIn(1.0f, 1.0f); // 정상 볼륨으로 자연스럽게 페이드인 재생
    }
}