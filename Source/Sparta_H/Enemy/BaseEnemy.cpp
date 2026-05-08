#include "BaseEnemy.h"

ABaseEnemy::ABaseEnemy()
{
	PrimaryActorTick.bCanEverTick = false;
	CurrentAlertLevel = EAlertLevel::Idle;
	bIsDead = false;
}

void ABaseEnemy::BeginPlay()
{
	Super::BeginPlay();
}

void ABaseEnemy::OnAlertLevelChanged(EAlertLevel NewLevel)
{
	if (bIsDead) return;
	CurrentAlertLevel = NewLevel;
	// AlertLevel 키 값을 업데이트하는 로직
}

void ABaseEnemy::Die()
{
	if (bIsDead) return;
	bIsDead = true;
	OnDeath.Broadcast();
}

float ABaseEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                             AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.f;

	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.f, MaxHealth);

	if (CurrentHealth <= 0.f)
	{
		Die();
	}

	return DamageAmount;
}
