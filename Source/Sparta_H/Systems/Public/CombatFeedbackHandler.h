#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatFeedbackHandler.generated.h"

// 처치 시 호출될 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnKillDelegate);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPARTA_H_API UCombatFeedbackHandler : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatFeedbackHandler();

	// 처치 시 호출
	void OnKill();

	// 캐릭터 팀원이 바인딩할 델리게이트
	UPROPERTY(BlueprintAssignable)
	FOnKillDelegate OnKillDelegate;
};