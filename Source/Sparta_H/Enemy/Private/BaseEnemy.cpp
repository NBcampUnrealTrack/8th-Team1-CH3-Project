#include "BaseEnemy.h"

ABaseEnemy::ABaseEnemy()
{
	PrimaryActorTick.bCanEverTick = false;
	CurrentAlertLevel = EAlertLevel::Idle;
	bIsDead = false;
	EnemyStatData = nullptr;
}

void ABaseEnemy::BeginPlay()
{
	Super::BeginPlay();
	InitializeStats();
}

void ABaseEnemy::InitializeStats()
{
	if (!EnemyStatData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] EnemyStatData가 할당되지 않았습니다. 기본값을 사용합니다."), *GetName());
		return;
	}

	MaxHealth     = EnemyStatData->MaxHealth;
	CurrentHealth = EnemyStatData->MaxHealth;
	Damage        = EnemyStatData->Damage;

	UE_LOG(LogTemp, Log, TEXT("[%s] InitializeStats - HP: %.0f / Damage: %.0f"),
		*GetName(), MaxHealth, Damage);
}

void ABaseEnemy::OnAlertLevelChanged(EAlertLevel NewLevel)
{
	if (bIsDead) return;
	CurrentAlertLevel = NewLevel;
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