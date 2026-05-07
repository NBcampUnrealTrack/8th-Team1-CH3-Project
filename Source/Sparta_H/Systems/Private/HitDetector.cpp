#include "HitDetector.h"

UHitDetector::UHitDetector()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UHitDetector::PerformLineTrace(const FVector& StartLocation, const FVector& Direction,float Distance,FHitResult& OutHitResult)
{
	FVector EndLocation = StartLocation + Direction * Distance;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());
	
	return GetWorld()->LineTraceSingleByChannel(OutHitResult, StartLocation, EndLocation, ECC_Visibility,Params);
}

EHitBone UHitDetector::IdentifyHitBone(FName BoneName)
{
	if (BoneName.IsNone())
	{
		return EHitBone::None;
	}
    
	static const TMap<FName, EHitBone> BoneMap =
	{
		// 머리
		{ TEXT("head"),         EHitBone::Head  },
		{ TEXT("neck_01"),      EHitBone::Head  },

		// 몸통
		{ TEXT("spine_01"),     EHitBone::Torso },
		{ TEXT("spine_02"),     EHitBone::Torso },
		{ TEXT("spine_03"),     EHitBone::Torso },
		{ TEXT("pelvis"),       EHitBone::Torso },

		// 팔
		{ TEXT("upperarm_l"),   EHitBone::Arm   },
		{ TEXT("upperarm_r"),   EHitBone::Arm   },
		{ TEXT("lowerarm_l"),   EHitBone::Arm   },
		{ TEXT("lowerarm_r"),   EHitBone::Arm   },

		// 다리
		{ TEXT("thigh_l"),      EHitBone::Leg   },
		{ TEXT("thigh_r"),      EHitBone::Leg   },
		{ TEXT("calf_l"),       EHitBone::Leg   },
		{ TEXT("calf_r"),       EHitBone::Leg   },
	};
    
	const EHitBone* Found = BoneMap.Find(BoneName);
	if (Found)
		return *Found;
	else
		return EHitBone::None;
}