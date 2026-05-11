#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.h"
#include "Blueprint/UserWidget.h"
#include "H_CrosshairWidget.generated.h"

class ASparta_HCharacter;

UCLASS()
class SPARTA_H_API UH_CrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 캐릭터로부터 현재 크로스헤어 상태를 가져오는 함수
	UFUNCTION(BlueprintCallable, Category = "Crosshair")
	ECrosshairState GetCurrentCrosshairState() const;

	UFUNCTION(BlueprintCallable, Category = "Crosshair")
	void SetCrosshairState(ECrosshairState NewState);

	// 조준 상태에 따른 부드러운 스케일링이나 색상 변화 로직을 이곳에 추가할 수 있습니다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Crosshair")
	void OnCrosshairStateChanged(ECrosshairState NewState);
};
