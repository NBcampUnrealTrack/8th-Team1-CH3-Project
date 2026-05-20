#include "UI/Public/H_WeaponDetailsWidget.h"
#include "Weapon/Public/WeaponDataAsset.h"
#include "Components/TextBlock.h"
#include "Curves/CurveFloat.h"

void UH_WeaponDetailsWidget::UpdateWeaponDetails(UWeaponDataAsset* WeaponData)
{
	// Modified: 그래프 로직 제거 및 텍스트 방식 롤백
	if (!WeaponData) return;

	// 무기 이름
	if (Text_WeaponName)
	{
		Text_WeaponName->SetText(WeaponData->WeaponName);
	}

	// 대미지
	if (Text_Damage)
	{
		FString DamageStr = FString::Printf(TEXT("대미지 : %.1f"), WeaponData->Damage);
		Text_Damage->SetText(FText::FromString(DamageStr));
	}

	// 거리 별 대미지 배율 (텍스트로 복구)
	if (Text_DistanceMultiplier)
	{
		if (WeaponData->DamageFalloffCurve)
		{
			float x10 = WeaponData->DamageFalloffCurve->GetFloatValue(1000.0f);  
			float x30 = WeaponData->DamageFalloffCurve->GetFloatValue(3000.0f);  
			float x50 = WeaponData->DamageFalloffCurve->GetFloatValue(5000.0f);  
			float x100 = WeaponData->DamageFalloffCurve->GetFloatValue(10000.0f); 

			FString DistStr = FString::Printf(TEXT("거리별 배율: 10m(x%.1f), 30m(x%.1f), 50m(x%.1f), 100m(x%.1f)"), 
											  x10, x30, x50, x100);
			Text_DistanceMultiplier->SetText(FText::FromString(DistStr));
		}
		else
		{
			Text_DistanceMultiplier->SetText(FText::FromString(TEXT("거리별 배율: 고정")));
		}
	}

	// 부위 별 대미지 배율
	if (Text_BodyPartMultiplier)
	{
		float HeadMult = WeaponData->BodyPartMultipliers.Contains("Head") ? WeaponData->BodyPartMultipliers["Head"] : 3.0f;
		float BodyMult = WeaponData->BodyPartMultipliers.Contains("Body") ? WeaponData->BodyPartMultipliers["Body"] : 1.0f;
		float LimbMult = WeaponData->BodyPartMultipliers.Contains("Limb") ? WeaponData->BodyPartMultipliers["Limb"] : 0.7f;
		
		FString PartStr = FString::Printf(TEXT("머리: x%.1f / 몸통: x%.1f / 팔·다리: x%.1f"), HeadMult, BodyMult, LimbMult);
		Text_BodyPartMultiplier->SetText(FText::FromString(PartStr));
	}

	// 소음 범위
	if (Text_SoundRange)
	{
		FString SoundStr = FString::Printf(TEXT("소음 범위 : %.0f m"), WeaponData->SoundRange / 100.0f);
		Text_SoundRange->SetText(FText::FromString(SoundStr));
	}

	// 연사력
	if (Text_FireRate)
	{
		float RPS = (WeaponData->FireRate > 0.0f) ? (1.0f / WeaponData->FireRate) : 0.0f;
		FString FireRateStr = FString::Printf(TEXT("연사력 : %.1f RPM"), RPS * 60.0f); 
		Text_FireRate->SetText(FText::FromString(FireRateStr));
	}

	// 장탄 수
	if (Text_MaxAmmo)
	{
		FString AmmoStr = FString::Printf(TEXT("장탄 수 : %d 발"), WeaponData->MaxAmmoCount);
		Text_MaxAmmo->SetText(FText::FromString(AmmoStr));
	}
}
