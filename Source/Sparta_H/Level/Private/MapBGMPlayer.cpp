#include "Level/Public/MapBGMPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "TimerManager.h"

AMapBGMPlayer::AMapBGMPlayer()
{
	PrimaryActorTick.bCanEverTick = false; // 불필요한 액터 틱 기능 비활성화
    CurrentBGMComponent = nullptr; // 오디오 컴포넌트 포인터 null 초기화
    PendingBGM = nullptr; // 대기 에셋 포인터 null 초기화
}

void AMapBGMPlayer::BeginPlay()
{
	Super::BeginPlay(); // 부모 클래스 초기화 호출

	if (DefaultBGM)
	{
		CurrentBGMComponent = UGameplayStatics::CreateSound2D(this, DefaultBGM, 1.0f, 1.0f, 0.0f, nullptr, false, true); // 초기 기본 사운드 컴포넌트 인스턴스 생성
        if (CurrentBGMComponent)
        {
            CurrentBGMComponent->FadeIn(1.0f, 1.0f); // 1초 동안 자연스럽게 페이드인 실행
        }
	}
}

void AMapBGMPlayer::TransitionBGM(USoundBase* NewBGM, float FadeOutTime, float DelayTime)
{
    PendingBGM = NewBGM; // 새 오디오 목표 에셋 등록

    if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying())
    {
        CurrentBGMComponent->FadeOut(FadeOutTime, 0.0f); // 재생 중인 사운드가 있으면 페이드아웃 처리
        GetWorldTimerManager().SetTimer(BGMTransitionTimerHandle, this, &AMapBGMPlayer::PlayNextBGMDeferred, DelayTime, false); // 타이머를 통해 지연 재생 예약
    }
    else
    {
        PlayNextBGMDeferred(); // 기존 재생 사운드가 없으면 즉시 새 사운드 재생 실행
    }
}

void AMapBGMPlayer::FadeOutCurrentBGM(float FadeTime)
{
	if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying())
	{
		CurrentBGMComponent->FadeOut(FadeTime, 0.0f); // 외부 엘리베이터 등에서 호출 시 기존 BGM 페이드아웃 처리
	}
}

void AMapBGMPlayer::FadeInNewBGM(USoundBase* NewBGM, float FadeTime)
{
	if (!NewBGM)
	{
		return; // 전달된 사운드 에셋이 없으면 명시적 반환 처리
	}

	if (CurrentBGMComponent)
	{
		CurrentBGMComponent->Stop(); // 리소스 점유 해제를 위해 이전 오디오 인스턴스 정지
	}

	CurrentBGMComponent = UGameplayStatics::CreateSound2D(this, NewBGM, 1.0f, 1.0f, 0.0f, nullptr, false, true); // 새 사운드 컴포넌트 인스턴스 할당
	if (CurrentBGMComponent)
	{
		CurrentBGMComponent->FadeIn(FadeTime, 1.0f); // 지정된 시간에 걸쳐 페이드인 재생
	}
}

void AMapBGMPlayer::PlayNextBGMDeferred()
{
    if (!PendingBGM)
    {
        return; // 예약된 대기 오디오 에셋이 유효하지 않으면 반환
    }

    if (CurrentBGMComponent)
    {
        CurrentBGMComponent->Stop(); // 리소스 혼선을 방지하기 위해 이전 오디오 컴포넌트 완전 정지
        CurrentBGMComponent = nullptr; // 컴포넌트 포인터 주소 명시적 초기화
    }

    CurrentBGMComponent = UGameplayStatics::CreateSound2D(this, PendingBGM, 1.0f, 1.0f, 0.0f, nullptr, false, true); // 예약된 배경음악으로 컴포넌트 동적 생성 및 등록

    if (CurrentBGMComponent)
    {
        CurrentBGMComponent->FadeIn(1.0f, 1.0f); // 오디오 컬링 현상을 방지하며 정상 정상 볼륨으로 페이드인 시작
    }
}