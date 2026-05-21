#include "Level/Public/BGMTransitionVolume.h"
#include "Level/Public/MapBGMPlayer.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

ABGMTransitionVolume::ABGMTransitionVolume()
{
    PrimaryActorTick.bCanEverTick = false; // 틱 비활성화
    
    TransitionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TransitionBox")); // 박스 컴포넌트 생성
    SetRootComponent(TransitionBox); // 루트 컴포넌트 지정
    
    TransitionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 오버랩 감지를 위한 충돌 연산 활성화
    TransitionBox->SetCollisionResponseToAllChannels(ECR_Overlap); // 모든 콜리전 채널에 대해 오버랩 허용
}

void ABGMTransitionVolume::BeginPlay()
{
    Super::BeginPlay(); // 부모 초기화 호출
    
    if (TransitionBox)
    {
        TransitionBox->OnComponentBeginOverlap.AddDynamic(this, &ABGMTransitionVolume::OnVolumeOverlapBegin); // 오버랩 이벤트 바인딩
    }
}

void ABGMTransitionVolume::OnVolumeOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (bHasTriggered || !NextBGM)
    {
        return; // 중복 실행 방지 및 에셋 유효성 검사
    }

    if (OtherActor && (OtherActor->IsA(ACharacter::StaticClass()) || OtherActor->GetInstigatorController() != nullptr))
    {
        TArray<AActor*> FoundBGMPlayers;
        UGameplayStatics::GetAllActorsOfClass(this, AMapBGMPlayer::StaticClass(), FoundBGMPlayers); // 맵에 배치된 BGM 플레이어 탐색
        
        if (FoundBGMPlayers.Num() > 0)
        {
            if (AMapBGMPlayer* BGMPlayer = Cast<AMapBGMPlayer>(FoundBGMPlayers[0]))
            {
                bHasTriggered = true; // 트리거 실행 플래그 활성화
                BGMPlayer->TransitionBGM(NextBGM, CrossfadeDuration, DelayBeforeStart); // BGM 전환 요청
                Destroy(); // 액터 소멸 처리
            }
        }
    }
}