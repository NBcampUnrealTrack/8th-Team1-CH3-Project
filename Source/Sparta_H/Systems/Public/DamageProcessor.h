#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CombatTypes.h"
#include "DamageDataAsset.h"
#include "DamageProcessor.generated.h"

UCLASS()
class SPARTA_H_API UDamageProcessor : public UObject
{
    GENERATED_BODY()

public:
    // 뼈 부위별 배율 (에디터에서 수정 가능)
    UPROPERTY(EditAnywhere, Category = "Damage")
    float BoneHead = 3.0f;

    UPROPERTY(EditAnywhere, Category = "Damage")
    float BoneTorso = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Damage")
    float BoneLimb = 0.7f;

    // 무기 종류별 데미지 에셋. 에디터에서 DA_Rifle / DA_Pistol
    UPROPERTY(EditAnywhere, Category = "Damage")
    TObjectPtr<UDamageDataAsset> RifleData;

    UPROPERTY(EditAnywhere, Category = "Damage")
    TObjectPtr<UDamageDataAsset> PistolData;

    float CalculateFinalDamage(const FCombatDamageInfo& Info);
    float GetDistanceMultiplier(float Distance, ECombatWeaponType WeaponType);
    float GetBoneMultiplier(EHitBone Bone);
};
