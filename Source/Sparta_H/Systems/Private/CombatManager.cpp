#include "CombatManager.h"
#include "Kismet/GameplayStatics.h"


UCombatManager::UCombatManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	HitDetector = CreateDefaultSubobject<UHitDetector>(TEXT("HitDetector"));
	DamageProcessor = CreateDefaultSubobject<UDamageProcessor>(TEXT("DamageProcessor"));
}

void UCombatManager::Fire(const FVector& AimStart, const FVector& AimDirection)
{
	// 1. 발사 소음 (발사 위치)
	EmitNoise(AimStart, FireNoiseRange);
	FHitResult HitResult;
	const bool bIsHit = HitDetector->PerformLineTrace(AimStart, AimDirection, TraceRange, HitResult);

	if (!bIsHit)
	{
		return; // 아무것도 안 맞았으면 종료
	}
	
	// 2. 대미지 정보 구성
	FCombatDamageInfo DamageInfo;
	DamageInfo.BaseDamage = BaseDamage;
	DamageInfo.Distance   = HitResult.Distance;
	DamageInfo.HitBone    = UHitDetector::IdentifyHitBone(HitResult.BoneName);

	// 3. 최종 대미지 계산
	const float FinalDamage = DamageProcessor->CalculateFinalDamage(DamageInfo);
	
	// 4. 피격 대상에 대미지 전달
	AActor* HitActor = HitResult.GetActor();
	if (IsValid(HitActor))
	{
		UGameplayStatics::ApplyDamage(
			HitActor,           // 맞은 액터
			FinalDamage,        // 최종 대미지
			nullptr,            // 가해자 컨트롤러 (나중에 연결)
			GetOwner(),         // 가해자 액터
			nullptr             // 대미지 타입 (기본값)
		);
	}
	
	// 5. 피격 소음 (피격 위치)
	EmitNoise(HitResult.ImpactPoint, HitNoiseRange);
}

void UCombatManager::EmitNoise(const FVector& NoiseLocation, float NoiseRange)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn)) return;

	OwnerPawn->MakeNoise(NoiseRange, OwnerPawn, NoiseLocation);
}