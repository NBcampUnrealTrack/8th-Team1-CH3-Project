#include "CombatManager.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "BaseEnemy.h"
#include "NiagaraFunctionLibrary.h"

UCombatManager::UCombatManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	HitDetector = CreateDefaultSubobject<UHitDetector>(TEXT("HitDetector"));
	DamageProcessor = CreateDefaultSubobject<UDamageProcessor>(TEXT("DamageProcessor"));
	FeedbackHandler = CreateDefaultSubobject<UCombatFeedbackHandler>(TEXT("FeedbackHandler"));
}

void UCombatManager::OnFire(const FVector& AimStart, const FVector& AimDirection, ECombatWeaponType WeaponType,
                            float BaseDamage, bool bTriggerAIAggro, UNiagaraSystem* ImpactVFX)
{
	// 칼은 별도 처리 (매니저 자체의 KnifeFront/KnifeBackDamage 사용)
	if (WeaponType == ECombatWeaponType::Knife)
	{
		KnifeAttack(AimStart, AimDirection, bTriggerAIAggro);
		return;
	}

	// 1. 발사 소음 (발사 위치) — 어그로 무기만
	if (bTriggerAIAggro)
	{
		EmitNoise(AimStart, GetFireNoiseRange(WeaponType));
	}

	// 2. 트레이스
	FHitResult HitResult;
	if (!HitDetector->PerformLineTrace(AimStart, AimDirection, TraceRange, HitResult))
	{
		return;
	}

	// ImpactVFX - 트레이스 성공 시 항상 표면 적중점에 스폰 (환경 / 적 무관)
	if (ImpactVFX != nullptr)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), ImpactVFX, HitResult.ImpactPoint,
			HitResult.ImpactNormal.Rotation());
	}
	
	AActor* HitActor = HitResult.GetActor();

	// 3. 액터 무효 또는 적이 아님 → 환경 오브젝트로 간주, 피격 소음만
	if (!IsValid(HitActor) || !HitActor->ActorHasTag("Enemy"))
	{
		if (bTriggerAIAggro)
		{
			EmitNoise(HitResult.ImpactPoint, GetHitNoiseRange(WeaponType));
		}
		return;
	}

	// 4. 적-vs-적 friendly fire 차단
	if (GetOwner()->ActorHasTag("Enemy")) return;

	// 5. 데미지 계산
	FCombatDamageInfo DamageInfo;
	DamageInfo.BaseDamage = BaseDamage;
	DamageInfo.Distance = HitResult.Distance;
	DamageInfo.HitBone = UHitDetector::IdentifyHitBone(HitResult.BoneName);
	DamageInfo.WeaponType = WeaponType;
	const float FinalDamage = DamageProcessor->CalculateFinalDamage(DamageInfo);

	// 6. 킬 피드백 등록
	RegisterKillFeedback(HitActor);

	// 7. 데미지 전달
	UGameplayStatics::ApplyDamage(HitActor, FinalDamage, nullptr, GetOwner(), nullptr);

	// 8. 피격 소음 (적 피격 위치)
	if (bTriggerAIAggro)
	{
		EmitNoise(HitResult.ImpactPoint, GetHitNoiseRange(WeaponType));
	}
}

void UCombatManager::EmitNoise(const FVector& NoiseLocation, float NoiseRange)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn)) return;

	UAISense_Hearing::ReportNoiseEvent(
		GetWorld(),
		NoiseLocation,
		1.f, // Loudness (0.0~1.0)
		OwnerPawn,
		NoiseRange, // 무기별 Range를 여기에 전달
		NAME_None
	);
}

float UCombatManager::GetFireNoiseRange(ECombatWeaponType WeaponType) const
{
	switch (WeaponType)
	{
	case ECombatWeaponType::Pistol: return FireNoiseRangePistol;
	case ECombatWeaponType::Rifle: return FireNoiseRangeRifle;
	case ECombatWeaponType::Knife: return FireNoiseRangeKnife;
	case ECombatWeaponType::Rock: return 0.f; // 0m
	default: return 0.f;
	}
}

float UCombatManager::GetHitNoiseRange(ECombatWeaponType WeaponType) const
{
	switch (WeaponType)
	{
	case ECombatWeaponType::Pistol: return HitNoiseRangePistol;
	case ECombatWeaponType::Rock: return HitNoiseRangeRock;
	default: return 0.f;
	}
}

void UCombatManager::KnifeAttack(const FVector& AimStart, const FVector& AimDirection, bool bTriggerAIAggro)
{
	FHitResult HitResult;
	if (!HitDetector->PerformLineTrace(AimStart, AimDirection, KnifeRange, HitResult)) return;

	AActor* HitActor = HitResult.GetActor();
	if (!IsValid(HitActor)) return;
	if (!HitActor->ActorHasTag("Enemy")) return;

	// 적-vs-적 friendly fire 차단
	if (GetOwner()->ActorHasTag("Enemy")) return;

	// 백/정면 판정 — 적의 정면 벡터와 (공격자→적) 벡터의 dot이 양수면 등 뒤에서 공격
	const FVector HitActorForward = HitActor->GetActorForwardVector();
	const FVector ToTarget = (HitActor->GetActorLocation() - GetOwner()->GetActorLocation()).GetSafeNormal();
	const float DotProduct = FVector::DotProduct(HitActorForward, ToTarget);
	const float KnifeDamage = (DotProduct > BackAttackThreshold) ? KnifeBackDamage : KnifeFrontDamage;

	// 킬 피드백 등록
	RegisterKillFeedback(HitActor);

	UGameplayStatics::ApplyDamage(HitActor, KnifeDamage, nullptr, GetOwner(), nullptr);

	// 칼은 GetHitNoiseRange가 0이라 어그로 거의 없지만, 플래그 일관성 유지
	if (bTriggerAIAggro)
	{
		EmitNoise(HitResult.ImpactPoint, GetHitNoiseRange(ECombatWeaponType::Knife));
	}
}

// 킬 피드백 등록
void UCombatManager::RegisterKillFeedback(AActor* HitActor)
{
	ABaseEnemy* Enemy = Cast<ABaseEnemy>(HitActor);
	if (IsValid(Enemy) && !Enemy->IsDead())
	{
		Enemy->OnDeath.AddUniqueDynamic(FeedbackHandler, &UCombatFeedbackHandler::OnKill);
	}
}
