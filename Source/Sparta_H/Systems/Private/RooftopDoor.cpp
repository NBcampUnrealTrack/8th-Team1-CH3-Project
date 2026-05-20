// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Public/RooftopDoor.h"
#include "PlayerCharacter.h"
#include "Engine/Engine.h"

ARooftopDoor::ARooftopDoor()
{
	// 부모 클래스인 ADoor의 생성자가 자동으로 호출되므로 기존 컴포넌트 구조가 안전하게 유지됩니다.
	bHasKey = false;
}

void ARooftopDoor::OpenDoor()
{
	// @brief 의도 수정: 열쇠를 먹었을 때 문이 혼자 열리지 않도록 오직 '열쇠 보유 플래그'만 true로 변경합니다.
	bHasKey = true;
	UE_LOG(LogTemp, Log, TEXT("RooftopDoor: 보스 열쇠 획득 상태가 감지되었습니다. 이제 문을 열 수 있습니다."));
}

bool ARooftopDoor::CanInteract_Implementation(APlayerCharacter* Interactor) const
{
	// @brief 열쇠 소지 여부와 상관없이 문 근처에 가면 무조건 Outline과 Glow 효과가 보이도록 항상 true를 반환합니다.
	return true;
}

FString ARooftopDoor::GetInteractionText_Implementation() const
{
	// @brief 열쇠 소지 상태에 따라 UI 가이드라인 문구를 유연하게 제어합니다.
	if (bHasKey)
	{
		return Super::GetInteractionText_Implementation();
	}
	else
	{
		return TEXT("열쇠가 필요합니다.");
	}
}

void ARooftopDoor::Interact_Implementation(APlayerCharacter* Interactor)
{
	if (!Interactor) return;

	// @brief 상호작용 키(F) 입력 시 열쇠를 가지고 있을 때만 부모의 문 회전 로직을 실행시킵니다.
	if (bHasKey)
	{
		// 부모 클래스(ADoor)에 구현된 원래의 문 열림 회전 및 틱 활성화 로직 실행
		Super::Interact_Implementation(Interactor);
	}
	else
	{
		// 열쇠가 없는 상태에서 F키를 누르면 문을 열지 않고 화면 중앙 상단에 빨간색 경고 메시지를 3초간 출력합니다.
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("열쇠를 찾아오세요!"));
		}
	}
}