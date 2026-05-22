#pragma once

#include "CoreMinimal.h"
#include "Level/Public/BaseInteractableActor.h"
#include "MissionRope.generated.h"

class APlayerCharacter;
class USoundBase;
class UCameraShakeBase;
class UAudioComponent;

// 최종 탈출용 밧줄 기믹 액터 클래스.
// 상호작용 시 인게임 HUD 비활성화, 슬로우 모션, 카메라 시점 제어, 포스트 프로세스 및 오디오 페이드 인 연출 후 최종 엔딩 HUD를 호출함.
UCLASS()
class SPARTA_H_API AMissionRope : public ABaseInteractableActor
{
    GENERATED_BODY()
    
public:    
    AMissionRope();
    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;

protected:
    // 미션 단계를 식별하기 위한 고유 ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Data")
    int32 MissionID = 0; 

    // 밧줄 흔들림 애니메이션 속도
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope Settings")
    float SwaySpeed;

    // 밧줄 흔들림 애니메이션 진폭
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope Settings")
    float SwayIntensity;

    // 시네마틱 연출 간 시점 전환용 타겟 카메라 액터
    UPROPERTY(EditAnywhere, Category = "Ending Cinematic")
    AActor* CinematicViewTarget;

    // 헬리콥터 탈출 사운드 에셋
    UPROPERTY(EditAnywhere, Category = "Ending Cinematic")
    USoundBase* HelicopterSound;

    // 카메라 전환 후 연출 지속 시간 (초)
    UPROPERTY(EditAnywhere, Category = "Ending Cinematic")
    float HelicopterViewDuration = 4.0f;

    // 헬리콥터 뷰에서 재생할 카메라 진동 효과
    UPROPERTY(EditAnywhere, Category = "Ending Cinematic")
    TSubclassOf<UCameraShakeBase> HelicopterCameraShake;

    // 센서 박스 오버랩 시작 시 직접 키 바인딩을 위한 오버라이드
    virtual void OnSensorOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

    // 센서 박스 오버랩 종료 시 직접 키 바인딩 해제를 위한 오버라이드
    virtual void OnSensorOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;

private:
    float RunningTime;
    bool bIsEndingCinematic = false;
    float CinematicElapsedTime = 0.0f;
    
    // F키 입력 상태를 확인하는 플래그
    bool bInputBound = false;

    UPROPERTY()
    UAudioComponent* HelicopterAudioComponent;

    // F키가 눌렸을 때 직접 상호작용을 실행하는 함수
    void OnInteractKeyPressed();

    // 시네마틱 2단계: 카메라 전환, 진동 및 화면 페이드 인 처리
    UFUNCTION()
    void Step2_ShowHelicopter(APlayerCharacter* Interactor);

    // 시네마틱 3단계: 연출 종료 및 게임 클리어 UI 직접 호출
    UFUNCTION()
    void Step3_FinishMission(APlayerCharacter* Interactor);

public:    
    virtual void Interact_Implementation(APlayerCharacter* Interactor) override;
    virtual bool CanInteract_Implementation(APlayerCharacter* Interactor) const override;
    virtual FString GetInteractionText_Implementation() const override;
};