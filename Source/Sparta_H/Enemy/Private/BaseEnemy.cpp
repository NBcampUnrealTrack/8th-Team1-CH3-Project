#include "BaseEnemy.h"
#include "Components/CapsuleComponent.h"

ABaseEnemy::ABaseEnemy()
{
	PrimaryActorTick.bCanEverTick = false;
	CurrentAlertLevel = EAlertLevel::Idle;
	bIsDead = false;
	EnemyStatData = nullptr;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void ABaseEnemy::BeginPlay()
{
	Super::BeginPlay();
	Tags.AddUnique(TEXT("Enemy"));
	InitializeStats();
}

void ABaseEnemy::InitializeStats()
{
	if (!EnemyStatData)
	{
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
