#include "CombatManager.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "BaseEnemy.h"

UCombatManager::UCombatManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	HitDetector = CreateDefaultSubobject<UHitDetector>(TEXT("HitDetector"));
	DamageProcessor = CreateDefaultSubobject<UDamageProcessor>(TEXT("DamageProcessor"));
	FeedbackHandler = CreateDefaultSubobject<UCombatFeedbackHandler>(TEXT("FeedbackHandler"));
}

void UCombatManager::OnFire(const FVector& AimStart, const FVector& AimDirection, ECombatWeaponType WeaponType,float BaseDamage,float BackAttackDamage )
{
	// 칼은 별도 처리
	if (WeaponType == ECombatWeaponType::Knife)
	{
		KnifeAttack(AimStart, AimDirection,BaseDamage, BackAttackDamage);
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
			
			
			ABaseEnemy* Enemy = Cast<ABaseEnemy>(HitActor);
			if (IsValid(Enemy) && !Enemy->IsDead())
			{
				Enemy->OnDeath.AddUniqueDynamic(
					FeedbackHandler, &UCombatFeedbackHandler::OnKill
				);
			}
			
			// 적 → 대미지 전달
			UGameplayStatics::ApplyDamage(
				HitActor,
				FinalDamage,
				nullptr,
				GetOwner(),
				nullptr
			);
		}
		else if (HitActor->ActorHasTag("Player"))
		{
			// 공격자가 적일 때만 플레이어에게 데미지
			if (!GetOwner()->ActorHasTag("Enemy")) return;

			UGameplayStatics::ApplyDamage(
				HitActor,
				FinalDamage,
				nullptr,
				GetOwner(),
				nullptr
			);
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
	
	UAISense_Hearing::ReportNoiseEvent(
		GetWorld(),
		NoiseLocation,
		1.f,           // Loudness (0.0~1.0)
		OwnerPawn,
		NoiseRange,    // 무기별 Range를 여기에 전달
		NAME_None
	);
}

float UCombatManager::GetFireNoiseRange(ECombatWeaponType WeaponType) const
{
	switch (WeaponType)
	{
	case ECombatWeaponType::Pistol: return 30000.f;   // 300m
	case ECombatWeaponType::Rifle:  return 150000.f;  // 1500m
	case ECombatWeaponType::Knife:  return 1000.f;    // 10m
	case ECombatWeaponType::Rock:   return 0.f;       // 0m
	default:                        return 0.f;
	}
}

float UCombatManager::GetHitNoiseRange(ECombatWeaponType WeaponType) const
{
	switch (WeaponType)
	{
	case ECombatWeaponType::Pistol: return HitNoiseRangePistol;
	case ECombatWeaponType::Rock:   return HitNoiseRangeRock;
	default:                        return 0.f;
	}
}

void UCombatManager::KnifeAttack(const FVector& AimStart, const FVector& AimDirection,float FrontDamage,float BackDamage)
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

	const float KnifeDamage = (DotProduct > 0.f) ? BackDamage : FrontDamage;
	
	ABaseEnemy* Enemy = Cast<ABaseEnemy>(HitActor);
	if (IsValid(Enemy) && !Enemy->IsDead())
	{
		Enemy->OnDeath.AddUniqueDynamic(
			FeedbackHandler, &UCombatFeedbackHandler::OnKill
		);
	}
	
	UGameplayStatics::ApplyDamage(
		HitActor, KnifeDamage, nullptr, GetOwner(), nullptr
	);

	EmitNoise(HitResult.ImpactPoint, GetHitNoiseRange(ECombatWeaponType::Knife));
}