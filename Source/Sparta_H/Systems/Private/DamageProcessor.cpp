#include "DamageProcessor.h"
#include "DamageDataAsset.h"

float UDamageProcessor::CalculateFinalDamage(const FCombatDamageInfo& Info, UDamageDataAsset* RifleDA, UDamageDataAsset* PistolDA)
{
    float DistMult = GetDistanceMultiplier(Info.Distance, Info.WeaponType, RifleDA, PistolDA);
    float BoneMult = GetBoneMultiplier(Info.HitBone);

    return Info.BaseDamage * DistMult * BoneMult;
}

float UDamageProcessor::GetDistanceMultiplier(float Distance, ECombatWeaponType WeaponType, UDamageDataAsset* RifleDA, UDamageDataAsset* PistolDA)
{
    UDamageDataAsset* SelectedData = nullptr;

    switch (WeaponType)
    {
    case ECombatWeaponType::Rifle:
        SelectedData = RifleDA;
        break;
    case ECombatWeaponType::Pistol:
        SelectedData = PistolDA;
        break;
    case ECombatWeaponType::Knife:
        return 1.0f;
    case ECombatWeaponType::Rock:
        return 1.0f;
    default:
        return 1.0f;
    }

    if (!IsValid(SelectedData))
    {
        UE_LOG(LogTemp, Warning, TEXT("DamageProcessor: DataAsset이 설정되지 않았습니다. WeaponType=%d"), (int32)WeaponType);
        return 1.f;
    }

    return SelectedData->GetMultiplierByDistance(Distance);
}

float UDamageProcessor::GetBoneMultiplier(EHitBone Bone)
{
    switch (Bone)
    {
    case EHitBone::Head:  return BoneHead;
    case EHitBone::Torso: return BoneTorso;
    case EHitBone::Arm:   return BoneLimb;
    case EHitBone::Leg:   return BoneLimb;
    case EHitBone::None:  return 1.0f;
    default:              return BoneTorso;
    }
}
