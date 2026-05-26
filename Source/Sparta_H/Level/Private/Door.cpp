#include "Level/Public/Door.h"
#include "PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

ADoor::ADoor()
{
    PrimaryActorTick.bCanEverTick = true; // 문 개폐 회전 보간을 위해 액터 틱 활성화
}

void ADoor::BeginPlay()
{
    // [정정] 부모 클래스의 BeginPlay를 반드시 '최우선'으로 호출합니다.
    // 이를 통해 부모의 원본 머티리얼 캐싱 및 컴포넌트 렌더 패스 등록이 완벽하게 완료된 상태에서 자식 로직이 안전하게 시작됩니다.
    Super::BeginPlay();

    if (MainMesh)
    {
        // 부모의 렌더 트리 정렬이 끝난 후 안전하게 초기 회전값을 세팅하므로 렌더링 잔상 고착 버그가 원천 차단됩니다.
        MainMesh->SetRelativeRotation(FRotator(0.f, ClosedYaw, 0.f));
    }

    SetActorTickEnabled(false); // 초기 상태 틱 연산 정지
}

void ADoor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime); // 부모 틱 연산 호출

    float TargetYaw = bIsOpening ? OpenedYaw : ClosedYaw; // 개폐 플래그에 따른 목표 Yaw 산출
    FRotator CurrentRotation = MainMesh->GetRelativeRotation();
    FRotator TargetRotation = FRotator(0.f, TargetYaw, 0.f);

    if (!CurrentRotation.Equals(TargetRotation, 0.1f))
    {
        // 계층 순서가 정 정상화되었으므로 꼬임 없이 부드럽고 정확하게 회전이 반영됩니다.
        MainMesh->SetRelativeRotation(FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, DoorSpeed));
    }
    else
    {
        SetActorTickEnabled(false); // 목표치 도달 시 틱 비활성화
    }
}

void ADoor::OnSensorOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 부모 클래스의 오버랩 로직을 정교하게 호출하여 머티리얼 전체 즉시 치환 구조를 발동합니다.
    Super::OnSensorOverlapBegin(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

void ADoor::OnSensorOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    // 부모 클래스의 오버랩 종료 로직을 호출하여 백업된 원본 머티리얼로 완벽하게 되돌립니다.
    Super::OnSensorOverlapEnd(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex);
}

void ADoor::Interact_Implementation(APlayerCharacter* Interactor) 
{ 
    ToggleDoor(); // 상호작용 입력 시 문 작동
}

bool ADoor::CanInteract_Implementation(APlayerCharacter* Interactor) const 
{ 
    return true; // 상호작용 가능 여부 항시 true
}

FString ADoor::GetInteractionText_Implementation() const 
{ 
    return TEXT("상호작용 (F)"); // 상호작용 안내 텍스트 반환
}

void ADoor::ToggleDoor() 
{ 
    bIsOpening = !bIsOpening; 
    SetActorTickEnabled(true); // 보간 처리를 위해 액터 틱 재가동

    USoundBase* SoundToPlay = bIsOpening ? OpenSound : CloseSound;
    if (SoundToPlay)
    {
        UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, GetActorLocation()); // 위치 기반 개폐 효과음 출력
    }
}