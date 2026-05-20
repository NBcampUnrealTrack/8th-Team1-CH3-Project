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

    float CalculateFinalDamage(const FCombatDamageInfo& Info, UDamageDataAsset* RifleDA, UDamageDataAsset* PistolDA);
    float GetDistanceMultiplier(float Distance, ECombatWeaponType WeaponType, UDamageDataAsset* RifleDA, UDamageDataAsset* PistolDA);
    float GetBoneMultiplier(EHitBone Bone);
};
