#include "H_ThrowableWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "PlayerCharacter.h"
#include "WeaponBase.h"
#include "WeaponDataAsset.h"
#include "Materials/MaterialInstanceDynamic.h"

void UH_ThrowableWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateUI();
}

void UH_ThrowableWidget::UpdateUI()
{
	APlayerCharacter* PC = GetOwningCharacter();
	if (!PC) return;

	AWeaponBase* Throwable = PC->GetThrowableWeapon();
	if (!Throwable)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::Visible);
	UWeaponDataAsset* Data = Throwable->GetWeaponData();
	if (!Data) return;

	// 1. 아이콘 설정
	if (ThrowableIcon)
	{
		ThrowableIcon->SetBrushFromSoftTexture(Data->WeaponIcon);
	}

	// 2. 쿨다운 진행도 (위에서 아래로)
	// CooldownOverlay가 "Filled" 방식의 Image이고 Fill Method가 Vertical
	float CooldownPercent = Throwable->GetThrowCooldownPercent();
	if (CooldownOverlay)
	{
		// 다이내믹 머티리얼을 사용한다면 파라미터 조절, 여기서는 단순 Percent
		CooldownOverlay->GetDynamicMaterial()->SetScalarParameterValue(TEXT("Percent"), CooldownPercent);
		
		// 쿨다운 중일 때만 보이게 하거나 항상 보이되 Percent가 0이면 투명하게 처리
		CooldownOverlay->SetVisibility(CooldownPercent > 0.0f ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	// 3. 수량 표시 (무한인 돌맹이는 숨김)
	if (StockCountText)
	{
		if (Throwable->HasUnlimitedStock())
		{
			StockCountText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			StockCountText->SetVisibility(ESlateVisibility::Visible);
			StockCountText->SetText(FText::AsNumber(Throwable->GetCurrentStock()));
		}
	}

	// 4. 활성화 반짝임 효과
	bool bIsOnCooldown = CooldownPercent > 0.0f;
	if (bWasOnCooldown && !bIsOnCooldown)
	{
		if (ActivationFlashAnim)
		{
			PlayAnimation(ActivationFlashAnim);
		}
	}
	bWasOnCooldown = bIsOnCooldown;
}

APlayerCharacter* UH_ThrowableWidget::GetOwningCharacter() const
{
	return Cast<APlayerCharacter>(GetOwningPlayerPawn());
}
