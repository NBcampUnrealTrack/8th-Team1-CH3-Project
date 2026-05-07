#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseEnemy.generated.h"

UENUM(BlueprintType)
enum class EAlertLevel : uint8
{
	CCTV = 0        UMETA(DisplayName = "CCTV"),
	Idle = 1        UMETA(DisplayName = "Idle"),
	Suspicious = 2  UMETA(DisplayName = "Suspicious"),
	Combat = 3      UMETA(DisplayName = "Combat"),
	Lost = 4        UMETA(DisplayName = "Lost")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathDelegate);

UCLASS()
class SPARTA_H_API ABaseEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	ABaseEnemy();

	// AI 감지 시 상태 변화 처리 함수
	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual void OnAlertLevelChanged(EAlertLevel NewLevel);
protected:
	virtual void BeginPlay() override;

	// 현재 경계 단계
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	EAlertLevel CurrentAlertLevel;
	
	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual void Die();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool bIsDead;
	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float CurrentHealth = 100.f;
	
public : 
	UFUNCTION(BlueprintCallable, Category = "AI")
	EAlertLevel GetCurrentAlertLevel() const { return CurrentAlertLevel; }
	
	UFUNCTION(BlueprintCallable, Category = "AI")
	bool IsDead() const { return bIsDead; }
	
	virtual float TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser) override;
	
	UPROPERTY(BlueprintAssignable)
	FOnDeathDelegate OnDeath;
};
