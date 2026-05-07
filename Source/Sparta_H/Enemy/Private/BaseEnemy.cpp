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

	// 스폰 시 타입에 맞는 스탯으로 초기화
	InitializeStats();
}

void ABaseEnemy::InitializeStats()
{
	switch (EnemyType)
	{
	case EEnemyType::Normal:
		MaxHealth     = 100.f;
		CurrentHealth = 100.f;
		break;

	case EEnemyType::Elite:
		MaxHealth     = 150.f;
		CurrentHealth = 150.f;
		break;

	default:
		break;
	}

	UE_LOG(LogTemp, Log, TEXT("[%s] InitializeStats - Type: %s / HP: %.0f"),
		*GetName(),
		EnemyType == EEnemyType::Elite ? TEXT("Elite") : TEXT("Normal"),
		MaxHealth);
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