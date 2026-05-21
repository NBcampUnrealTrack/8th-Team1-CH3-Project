#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "H_GameMode.generated.h"

class UAudioComponent;
class USoundBase;

UCLASS(minimalapi)
class AH_GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AH_GameMode();

	// 전역 BGM 크로스페이드 전환 요청 함수
	void TransitionBGM(USoundBase* NewBGM, float FadeOutDuration, float DelayDuration);

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BGM Settings")
	USoundBase* InitialBGM; // 맵 시작 시 최초로 재생할 기본 배경음악 에셋

private:
	// 지연 시간 완료 후 새 BGM을 페이드인하는 내부 예약 함수
	void PlayNextBGMDeferred();

	UPROPERTY()
	UAudioComponent* CurrentBGMComponent; // 현재 재생 중인 BGM 컴포넌트 인스턴스 주소 포인터

	UPROPERTY()
	USoundBase* PendingBGM; // 재생을 대기 중인 다음 목표 BGM 에셋 포인터

	FTimerHandle BGMTransitionTimerHandle; // 페이드아웃 및 재생 지연 동기화용 타이머 핸들
};