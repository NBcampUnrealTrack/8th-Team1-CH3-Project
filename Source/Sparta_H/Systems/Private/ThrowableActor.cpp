#include "ThrowableActor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"
#include "Perception/AISense_Hearing.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"

AThrowableActor::AThrowableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetNotifyRigidBodyCollision(true);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 1500.f;
	ProjectileMovement->MaxSpeed = 1500.f;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 1.0f;
}

void AThrowableActor::BeginPlay()
{
	Super::BeginPlay();
	MeshComponent->OnComponentHit.AddDynamic(this, &AThrowableActor::HandleOnHit);
}

void AThrowableActor::HandleOnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                                  UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ThrowableType == ECombatWeaponType::Grenade)
	{
		const FVector ExplosionLocation = GetActorLocation();

		// 폭발 이펙트 재생
		if (ExplosionEffect)
		{
			UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(), ExplosionEffect, ExplosionLocation,
				FRotator::ZeroRotator, FVector::OneVector, true, true);

			if (NiagaraComp)
			{
				TWeakObjectPtr<UNiagaraComponent> WeakComp(NiagaraComp);
				FTimerHandle EffectTimerHandle;
				GetWorld()->GetTimerManager().SetTimer(EffectTimerHandle, FTimerDelegate::CreateLambda([WeakComp]()
				{
					if (WeakComp.IsValid())
					{
						WeakComp->DeactivateImmediate();
					}
				}), 3.0f, false);
			}
		}
		if (ExplosionSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, ExplosionLocation);
		}

		// Instigator 팀으로 피해 대상 결정: 적이 던졌으면 플레이어를, 플레이어가 던졌으면 적을 타격
		const bool bThrownByEnemy = GetInstigator() && GetInstigator()->ActorHasTag("Enemy");
		const FName TargetTag = bThrownByEnemy ? FName("Player") : FName("Enemy");

		UE_LOG(LogTemp, Log, TEXT("[Grenade] 폭발 | 위치=(%.0f,%.0f,%.0f) | 반경=%.0f | 타겟팀=%s"),
			ExplosionLocation.X, ExplosionLocation.Y, ExplosionLocation.Z, ExplosionRadius, *TargetTag.ToString());

		// 범위 안 액터 탐색
		TArray<FOverlapResult> Overlaps;
		FCollisionShape Sphere = FCollisionShape::MakeSphere(ExplosionRadius);
		GetWorld()->OverlapMultiByChannel(
			Overlaps,
			ExplosionLocation,
			FQuat::Identity,
			ECC_Pawn,
			Sphere
		);

		for (FOverlapResult& Overlap : Overlaps)
		{
			AActor* Target = Overlap.GetActor();
			if (!IsValid(Target)) continue;
			if (!Target->ActorHasTag(TargetTag)) continue;

			const float Distance = FVector::Dist(ExplosionLocation, Target->GetActorLocation()) / 100.f;

			float Multiplier = 0.f;
			if (Distance <= 2.f) Multiplier = 1.0f;
			else if (Distance <= 4.f) Multiplier = 0.7f;
			else if (Distance <= 6.f) Multiplier = 0.5f;
			else Multiplier = 0.f;

			if (Multiplier <= 0.f) continue;

			const float FinalDamage = ExplosionDamage * Multiplier;
			UE_LOG(LogTemp, Log, TEXT("[Grenade] 피해 → %s | 거리=%.1fm | 배율=%.1f | 데미지=%.0f"),
				*Target->GetName(), Distance, Multiplier, FinalDamage);
			UGameplayStatics::ApplyDamage(Target, FinalDamage, nullptr, this, nullptr);
		}
		Destroy();
	}
	else if (ThrowableType == ECombatWeaponType::Rock)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Rock] HandleOnHit 호출 | OtherActor=%s | Instigator=%s"),
			OtherActor ? *OtherActor->GetName() : TEXT("NULL"),
			GetInstigator() ? *GetInstigator()->GetName() : TEXT("NULL"));

		if (IsValid(OtherActor) && OtherActor->ActorHasTag("Enemy"))
		{
			UGameplayStatics::ApplyDamage(
				OtherActor, 1.f, nullptr, this, nullptr
			);
		}

		UAISense_Hearing::ReportNoiseEvent(
			GetWorld(),
			GetActorLocation(),
			1.f,
			GetInstigator(),
			RockNoiseRange,
			FName("Rock")
		);

		UE_LOG(LogTemp, Warning, TEXT("[Rock] ReportNoiseEvent 완료 | 위치=%s | 범위=%.0f | Instigator=%s"),
			*GetActorLocation().ToString(), RockNoiseRange,
			GetInstigator() ? *GetInstigator()->GetName() : TEXT("NULL"));
		
		// 재충돌 방지
		MeshComponent->OnComponentHit.RemoveAll(this);
		if (ProjectileMovement)
		{
			ProjectileMovement->StopMovementImmediately();
			ProjectileMovement->Deactivate();
		}
		SetLifeSpan(2.0); // 2초 후 자동 소멸
	}
}

void AThrowableActor::Launch(const FVector& Direction, float Speed)
{
	if (ProjectileMovement == nullptr)
	{
		return;
	}

	// Speed <= 0 이면 DA 차징 미사용 호출로 간주하고 컴포넌트 기본 속도 사용
	const float EffectiveSpeed = Speed > 0.0f ? Speed : ProjectileMovement->InitialSpeed;

	// 궤적 예측 결과와 일치시키기 위해 InitialSpeed/MaxSpeed 도 동기화
	ProjectileMovement->InitialSpeed = EffectiveSpeed;
	ProjectileMovement->MaxSpeed = EffectiveSpeed;
	ProjectileMovement->Velocity = Direction * EffectiveSpeed;
}
