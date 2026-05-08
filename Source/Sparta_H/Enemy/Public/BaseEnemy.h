#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseEnemy.generated.h"

// ---------------------------------------------------------------
// AlertLevel (경계 단계)
// ---------------------------------------------------------------
UENUM(BlueprintType)
enum class EAlertLevel : uint8
{
	CCTV       = 0  UMETA(DisplayName = "CCTV"),
	Idle       = 1  UMETA(DisplayName = "Idle"),
	Suspicious = 2  UMETA(DisplayName = "Suspicious"),
	Combat     = 3  UMETA(DisplayName = "Combat"),
	Lost       = 4  UMETA(DisplayName = "Lost")
};

// ---------------------------------------------------------------
// EnemyType (기획서: Normal / Elite)
// ---------------------------------------------------------------
UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	Normal  UMETA(DisplayName = "Normal"),   // HP 100 / Damage 15
	Elite   UMETA(DisplayName = "Elite")     // HP 150 / Damage 20
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathDelegate);

UCLASS()
class SPARTA_H_API ABaseEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	ABaseEnemy();

	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual void OnAlertLevelChanged(EAlertLevel NewLevel);

protected:
	virtual void BeginPlay() override;

	// 타입에 따라 MaxHealth / CurrentHealth 초기화
	// 자식 클래스에서 override해 WeaponDamage 등 추가 스탯도 설정 가능
	virtual void InitializeStats();

	// ---------------------------------------------------------------
	// 적 타입 (에디터에서 Normal / Elite 선택)
	// ---------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stats")
	EEnemyType EnemyType = EEnemyType::Normal;

	// ---------------------------------------------------------------
	// 경계 단계
	// ---------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	EAlertLevel CurrentAlertLevel;

	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual void Die();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool bIsDead;

	// ---------------------------------------------------------------
	// 체력 (InitializeStats에서 타입별로 설정됨)
	// ---------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stats")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Stats")
	float CurrentHealth = 100.f;

public:
	UFUNCTION(BlueprintCallable, Category = "AI")
	EAlertLevel GetCurrentAlertLevel() const { return CurrentAlertLevel; }

	UFUNCTION(BlueprintCallable, Category = "AI")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintCallable, Category = "AI|Stats")
	EEnemyType GetEnemyType() const { return EnemyType; }

	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

	UPROPERTY(BlueprintAssignable)
	FOnDeathDelegate OnDeath;
};