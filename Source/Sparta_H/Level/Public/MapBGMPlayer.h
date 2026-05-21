#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapBGMPlayer.generated.h"

class UAudioComponent;
class USoundBase;

UCLASS()
class SPARTA_H_API AMapBGMPlayer : public AActor
{
	GENERATED_BODY()
    
public:    
	AMapBGMPlayer();

	// BGM 전환을 위한 통합 제어 함수
	void TransitionBGM(USoundBase* NewBGM, float FadeOutTime, float DelayTime);

	// Elevator 등 외부 클래스와의 하위 호환성을 위한 레거시 인터페이스 함수
	void FadeOutCurrentBGM(float FadeTime);
	void FadeInNewBGM(USoundBase* NewBGM, float FadeTime);

protected:
	virtual void BeginPlay() override;

private:
	// 지연 후 새 BGM을 재생하는 내부 예약 함수
	void PlayNextBGMDeferred();

	UPROPERTY(EditAnywhere, Category = "BGM Settings")
	USoundBase* DefaultBGM; // 에디터에서 설정할 기본 배경음악 에셋

	UPROPERTY()
	UAudioComponent* CurrentBGMComponent; // 현재 재생 중인 오디오 컴포넌트의 인스턴스 포인터

	UPROPERTY()
	USoundBase* PendingBGM; // 재생을 대기 중인 다음 배경음악 에셋 포인터

	FTimerHandle BGMTransitionTimerHandle; // 전환 지연 타이머 제어용 핸들
};