#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "H_WeaponDetailsWidget.generated.h"

class UWeaponDataAsset;
class UTextBlock;

/**
 * 인벤토리에서 무기의 상세 정보를 표시하는 위젯 클래스입니다.
 */
UCLASS()
class SPARTA_H_API UH_WeaponDetailsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 무기 데이터를 기반으로 UI 내용을 갱신합니다.
	// 무기 상세 정보 업데이트 함수 추가
	UFUNCTION(BlueprintCallable, Category = "UI|Weapon")
	void UpdateWeaponDetails(UWeaponDataAsset* WeaponData);

protected:
	// ───── 위젯 바인딩 (WBP에서 동일한 이름의 위젯이 필요합니다) ─────
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_WeaponName;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_Damage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_DistanceMultiplier;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_BodyPartMultiplier;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_SoundRange;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_FireRate;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_MaxAmmo;
};
