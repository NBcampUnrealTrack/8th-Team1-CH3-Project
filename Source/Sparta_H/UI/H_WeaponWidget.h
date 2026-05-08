#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "H_WeaponWidget.generated.h"

UCLASS()
class SPARTA_H_API UH_WeaponWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	// Modified: 캐릭터 정보를 받아 무기 데이터를 스스로 업데이트
	UFUNCTION(BlueprintCallable, Category = "UI|Weapon")
	void UpdateFromCharacter(class ASparta_HCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "UI|Weapon")
	void UpdateWeaponInfo(const FString& WeaponName, UTexture2D* WeaponIcon, int32 CurrentAmmo, int32 MaxAmmo);

protected:
	// Modified: 블루프린트 위젯 바인딩 변수들 추가
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* WeaponNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* WeaponIconImage;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* AmmoText;
};
