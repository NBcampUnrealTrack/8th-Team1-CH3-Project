#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyStatDataAsset.h"
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathDelegate);

UCLASS()
class SPARTA_H_API ABaseEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    ABaseEnemy();

    UFUNCTION(BlueprintCallable, Category = "AI")
    virtual void OnAlertLevelChanged(EAlertLevel NewLevel);

    // 경계 상태 변화를 블루프린트에서 처리하기 위한 이벤트
    UFUNCTION(BlueprintImplementableEvent, Category = "AI", meta = (DisplayName = "OnAlertLevelChanged"))
    void OnAlertLevelChanged_BP(EAlertLevel NewLevel);

protected:
    virtual void BeginPlay() override;

    // 데이터 에셋 기반 스탯 초기화
    // 자식 클래스에서 override해 WeaponDamage 등 추가 스탯도 설정 가능
    virtual void InitializeStats();

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
    // 적 스탯 데이터 에셋 (에디터에서 DA_Enemy_Normal / DA_Enemy_Elite 할당)
    // ---------------------------------------------------------------
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Stats")
    UEnemyStatDataAsset* EnemyStatData;

    // ---------------------------------------------------------------
    // 체력 (InitializeStats에서 EnemyStatData 기반으로 설정됨)
    // ---------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stats")
    float MaxHealth = 100.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Stats")
    float CurrentHealth = 100.f;

    // 기본 공격력 (InitializeStats에서 설정됨)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Stats")
    float Damage = 15.f;

public:
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