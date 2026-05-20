#include "UI/Public/H_WeaponDetailsWidget.h"
#include "Weapon/Public/WeaponDataAsset.h"
#include "Components/TextBlock.h"
#include "Curves/CurveFloat.h"

void UH_WeaponDetailsWidget::UpdateWeaponDetails(UWeaponDataAsset* WeaponData)
{
	// Modified: 무기 데이터 에셋을 위젯 텍스트로 변환하여 표시
	if (!WeaponData) return;

	// 무기 이름
	if (Text_WeaponName)
	{
		Text_WeaponName->SetText(WeaponData->WeaponName);
	}

	// 대미지
	if (Text_Damage)
	{
		Text_Damage->SetText(FText::AsNumber(WeaponData->Damage));
	}

	// 거리 별 대미지 배율 (대표값 예시: 10m, 50m)
	if (Text_DistanceMultiplier)
	{
		if (WeaponData->DamageFalloffCurve)
		{
			float Mult10m = WeaponData->DamageFalloffCurve->GetFloatValue(1000.0f); // 1000cm = 10m
			float Mult50m = WeaponData->DamageFalloffCurve->GetFloatValue(5000.0f); // 5000cm = 50m
			FString DistStr = FString::Printf(TEXT("10m: x%.1f / 50m: x%.1f"), Mult10m, Mult50m);
			Text_DistanceMultiplier->SetText(FText::FromString(DistStr));
		}
		else
		{
			Text_DistanceMultiplier->SetText(FText::FromString(TEXT("고정 대미지")));
		}
	}

	// 부위 별 대미지 배율 (Head, Body 배율 추출)
	if (Text_BodyPartMultiplier)
	{
		float HeadMult = WeaponData->BodyPartMultipliers.Contains("Head") ? WeaponData->BodyPartMultipliers["Head"] : 1.0f;
		float BodyMult = WeaponData->BodyPartMultipliers.Contains("Body") ? WeaponData->BodyPartMultipliers["Body"] : 1.0f;
		FString PartStr = FString::Printf(TEXT("헤드: x%.1f / 바디: x%.1f"), HeadMult, BodyMult);
		Text_BodyPartMultiplier->SetText(FText::FromString(PartStr));
	}

	// 소음 범위
	if (Text_SoundRange)
	{
		FString SoundStr = FString::Printf(TEXT("%.0fm"), WeaponData->SoundRange / 100.0f);
		Text_SoundRange->SetText(FText::FromString(SoundStr));
	}

	// 연사력 (초당 발사 수로 계산: 1 / FireRate)
	if (Text_FireRate)
	{
		float RPS = (WeaponData->FireRate > 0.0f) ? (1.0f / WeaponData->FireRate) : 0.0f;
		FString FireRateStr = FString::Printf(TEXT("%.1f RPM"), RPS * 60.0f); // 분당 발사 수(RPM)로 표시
		Text_FireRate->SetText(FText::FromString(FireRateStr));
	}

	// 장탄 수
	if (Text_MaxAmmo)
	{
		Text_MaxAmmo->SetText(FText::AsNumber(WeaponData->MaxAmmoCount));
	}
}
