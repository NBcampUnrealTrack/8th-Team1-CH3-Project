#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "H_TimerWidget.generated.h"

UCLASS()
class SPARTA_H_API UH_TimerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI|Timer")
	void UpdateTimer(float RemainingTime);

protected:
	// 블루프린트에서 바인딩할 텍스트 위젯
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* TimerText;
};
