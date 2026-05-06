#include "CombatManager.h"
#include "Kismet/GameplayStatics.h"


UCombatManager::UCombatManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	HitDetector = CreateDefaultSubobject<UHitDetector>(TEXT("HitDetector"));
	DamageProcessor = CreateDefaultSubobject<UDamageProcessor>(TEXT("DamageProcessor"));
	FeedbackHandler = CreateDefaultSubobject<UCombatFeedbackHandler>(TEXT("FeedbackHandler"));
}

void UCombatManager::OnFire(const FVector& AimStart, const FVector& AimDirection, ECombatWeaponType WeaponType)
{
	// 칼은 별도 처리
	if (WeaponType == ECombatWeaponType::Knife)
	{
		KnifeAttack(AimStart, AimDirection);
		return;
	}
	
	// 1. 발사 소음 (발사 위치)
	EmitNoise(AimStart, GetFireNoiseRange(WeaponType));
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
	DamageInfo.WeaponType  = WeaponType; 

	// 3. 최종 대미지 계산
	const float FinalDamage = DamageProcessor->CalculateFinalDamage(DamageInfo);
	
	// 4. 피격 대상에 대미지 전달
	AActor* HitActor = HitResult.GetActor();
	
	if (IsValid(HitActor))
	{
		if (HitActor->ActorHasTag("Enemy"))
		{
			// 공격자도 적이면 무효
			if (GetOwner()->ActorHasTag("Enemy")) return;
			
			// 적 → 대미지 전달
			UGameplayStatics::ApplyDamage(
				HitActor,
				FinalDamage,
				nullptr,
				GetOwner(),
				nullptr
			);
			
			// 처치 확인 후 OnKill 호출
			if (!IsValid(HitActor) || HitActor->IsActorBeingDestroyed())
			{
				FeedbackHandler->OnKill();
			}
		}
		else
		{
			// 환경 오브젝트 → 소음만 발생
			EmitNoise(HitResult.ImpactPoint, GetHitNoiseRange(WeaponType));
			return;
		}
	}

	// 5. 피격 소음 (피격 위치)
	EmitNoise(HitResult.ImpactPoint, GetHitNoiseRange(WeaponType));
}

void UCombatManager::EmitNoise(const FVector& NoiseLocation, float NoiseRange)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn)) return;

	OwnerPawn->MakeNoise(NoiseRange, OwnerPawn, NoiseLocation);
}

float UCombatManager::GetFireNoiseRange(ECombatWeaponType WeaponType) const
{
	switch (WeaponType)
	{
	case ECombatWeaponType::Pistol: return 30000.f;  // 300m
	case ECombatWeaponType::Rifle:  return 30000.f;  // 300m
	case ECombatWeaponType::Knife:  return 1000.f;   // 10m
	case ECombatWeaponType::Rock:   return 0.f;      // 0m
	default:                        return 0.f;
	}
}

float UCombatManager::GetHitNoiseRange(ECombatWeaponType WeaponType) const
{
	switch (WeaponType)
	{
	case ECombatWeaponType::Pistol: return 2000.f;   // 20m
	case ECombatWeaponType::Rifle:  return 2000.f;   // 20m
	case ECombatWeaponType::Rock:   return 10000.f;  // 100m
	default:                        return 0.f;
	}
}

void UCombatManager::KnifeAttack(const FVector& AimStart, const FVector& AimDirection)
{
	FHitResult HitResult;
	const bool bIsHit = HitDetector->PerformLineTrace(AimStart, AimDirection, KnifeRange, HitResult);

	if (!bIsHit) return;

	AActor* HitActor = HitResult.GetActor();
	if (!IsValid(HitActor)) return;

	// 적이 아니면 무시
	if (!HitActor->ActorHasTag("Enemy")) return;
	// 공격자도 적이면 무효
	if (GetOwner()->ActorHasTag("Enemy")) return;

	// 뒤에서 공격 여부 체크
	const FVector HitActorForward = HitActor->GetActorForwardVector();
	const FVector ToTarget = (HitActor->GetActorLocation() - GetOwner()->GetActorLocation()).GetSafeNormal();
	const float DotProduct = FVector::DotProduct(HitActorForward, ToTarget);

	const float KnifeDamage = (DotProduct > 0.f) ? KnifeBackDamage : KnifeFrontDamage;

	UGameplayStatics::ApplyDamage(
		HitActor, KnifeDamage, nullptr, GetOwner(), nullptr
	);
	
	// 처치 확인 후 OnKill 호출
	if (!IsValid(HitActor) || HitActor->IsActorBeingDestroyed())
	{
		FeedbackHandler->OnKill();
	}

	EmitNoise(HitResult.ImpactPoint, GetHitNoiseRange(ECombatWeaponType::Knife));
}