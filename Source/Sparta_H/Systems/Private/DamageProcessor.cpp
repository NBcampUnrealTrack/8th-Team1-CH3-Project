// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageProcessor.h"

float UDamageProcessor::CalculateFinalDamage(const FCombatDamageInfo& Info)
{
	float DistMult = GetDistanceMultiplier(Info.Distance, Info.WeaponType);
	float BoneMult = GetBoneMultiplier(Info.HitBone);

	return Info.BaseDamage * DistMult * BoneMult;
}

float UDamageProcessor::GetDistanceMultiplier(float Distance, ECombatWeaponType WeaponType)
{
	switch (WeaponType)
	{
	case ECombatWeaponType::Rifle:
	case ECombatWeaponType::Pistol:
		{
			if (GUN_RANGE_T1 >= Distance)
				return GUN_MULT_T1;
			else if (GUN_RANGE_T2 >= Distance)
				return GUN_MULT_T2;
			else if (GUN_RANGE_T3 >= Distance)
				return GUN_MULT_T3;
			else if (GUN_RANGE_T4 >= Distance)
				return GUN_MULT_T4;
			else
				return GUN_MULT_T5;
		}
	case ECombatWeaponType::Knife: return 1.0f;
	case ECombatWeaponType::Rock: return 1.0f;
	default: return 1.0f;
	}
}

float UDamageProcessor::GetBoneMultiplier(EHitBone Bone)
{
	switch (Bone)
	{
	case EHitBone::Head: return BoneHead;
	case EHitBone::Torso: return BoneTorso;
	case EHitBone::Arm: return BoneLimb;
	case EHitBone::Leg: return BoneLimb;
	default: return BoneTorso;
	}
}
