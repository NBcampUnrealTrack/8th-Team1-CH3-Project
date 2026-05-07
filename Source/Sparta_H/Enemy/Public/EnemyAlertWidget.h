#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BaseEnemy.h"
#include "EnemyAlertWidget.generated.h"

/**
 * 적 머리 위에 표시되는 경계 상태 아이콘 위젯
 * 
 * 사용법:
 * 1. 이 클래스를 부모로 WBP_EnemyAlertIcon 블루프린트 생성
 * 2. WBP 내에서 OnAlertLevelUpdated 이벤트 구현
 *    - Idle       → 위젯 전체 숨김
 *    - Suspicious → "??" 텍스트/이미지 표시
 *    - Combat     → "!!" 텍스트/이미지 표시
 *    - Lost       → "!!" 텍스트/이미지 표시
 */
UCLASS()
class SPARTA_H_API UEnemyAlertWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// EnemyCharacter의 OnAlertLevelChanged()에서 호출
	// 블루프린트에서 아이콘 전환 로직 구현
	UFUNCTION(BlueprintImplementableEvent, Category = "Alert")
	void OnAlertLevelUpdated(EAlertLevel NewLevel);
};