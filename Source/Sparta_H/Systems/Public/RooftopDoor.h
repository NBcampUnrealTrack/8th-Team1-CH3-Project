// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Level/Public/Door.h"
#include "RooftopDoor.generated.h"

/**
 * @class ARooftopDoor
 * @brief 기존 Door 기능을 상속받되, 열쇠가 있어야만 상호작용이 가능한 옥상 전용 문
 */
UCLASS()
class SPARTA_H_API ARooftopDoor : public ADoor
{
	GENERATED_BODY()

public:
	ARooftopDoor();

	// BossKey.cpp에서 열쇠를 획득했을 때 호출하는 함수 (자동 열림 방지 처리 완료)
	UFUNCTION(BlueprintCallable, Category = "Door Settings")
	void OpenDoor();

protected:
	// 열쇠 소지 여부를 결정하는 변수 (에디터 디테일 패널의 Door Settings 카테고리에서 체크 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Settings")
	bool bHasKey = false;

public:
	// 다른 문들과 동일하게 하이라이트(Outline, Glow) 효과를 켜기 위한 상호작용 조건 오버라이드
	virtual bool CanInteract_Implementation(class APlayerCharacter* Interactor) const override;

	// 상호작용 키(F) 입력 시 실행될 로직 오버라이드 (열쇠 소지 시에만 문 회전)
	virtual void Interact_Implementation(class APlayerCharacter* Interactor) override;

	// UI에 표시될 상호작용 가이드 텍스트 오버라이드
	virtual FString GetInteractionText_Implementation() const override;
};