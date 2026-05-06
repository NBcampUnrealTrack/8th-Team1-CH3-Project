#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "H_PlayerController.generated.h"

class UH_HUDWidget;

UCLASS()
class SPARTA_H_API AH_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// 에디터에서 지정할 위젯 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UH_HUDWidget> HUDWidgetClass;

	// 생성된 위젯 인스턴스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UH_HUDWidget> HUDWidgetInstance;

protected:
	virtual void BeginPlay() override;
};
