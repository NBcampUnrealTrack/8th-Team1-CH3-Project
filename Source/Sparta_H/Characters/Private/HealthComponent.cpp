#include "HealthComponent.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CurrentHealth = MaxHealth;
}


// Called when the game starts
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// 액터의 소유주에 이벤트 함수 바인딩
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
	}
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType,
                                           class AController* InstigatedBy, AActor* DamageCauser)
{
	UE_LOG(LogTemp, Warning, TEXT("[HealthComponent] HandleTakeAnyDamage 호출 | HP=%.0f/%.0f | Damage=%.1f | Causer=%s"),
		CurrentHealth, MaxHealth, Damage,
		DamageCauser ? *DamageCauser->GetName() : TEXT("NULL"));

	if (Damage <= 0.f || CurrentHealth <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HealthComponent] 데미지 무시 — Damage=%.1f, HP=%.0f"), Damage, CurrentHealth);
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.f, MaxHealth);

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, DamageType);

	if (CurrentHealth <= 0.f)
	{
		OnDeath.Broadcast();
	}
}

float UHealthComponent::SetHealth(float NewHealth)
{
	CurrentHealth = FMath::Clamp(NewHealth, 0.f, MaxHealth);
	return CurrentHealth;
}
