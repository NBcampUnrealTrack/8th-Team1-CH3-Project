#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BGMTransitionVolume.generated.h"

UCLASS()
class SPARTA_H_API ABGMTransitionVolume : public AActor
{
    GENERATED_BODY()
    
public:    
    ABGMTransitionVolume();

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnVolumeOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

protected:
    UPROPERTY(VisibleAnywhere, Category = "Components")
    class UBoxComponent* TransitionBox; // 플레이어 진입을 감지하는 박스 컴포넌트

    UPROPERTY(EditAnywhere, Category = "BGM Settings")
    class USoundBase* NextBGM; // 전환하여 재생할 목표 배경음악 에셋

    UPROPERTY(EditAnywhere, Category = "BGM Settings")
    float CrossfadeDuration = 3.0f; // 기존 BGM 페이드아웃에 소요되는 시간

    UPROPERTY(EditAnywhere, Category = "BGM Settings")
    float DelayBeforeStart = 2.0f; // 새 BGM 재생 지연 시간

private:
    bool bHasTriggered = false; // 중복 크로스페이드 호출 방지 플래그
};