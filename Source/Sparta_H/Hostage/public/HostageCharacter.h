#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MissionInteractableInterface.h"
#include "HostageCharacter.generated.h"

class APlayerCharacter;

UENUM(BlueprintType)
enum class EHostageState : uint8
{
	Stay       = 0  UMETA(DisplayName = "Stay"),
	Following  = 1  UMETA(DisplayName = "Following"),
	Dead       = 2  UMETA(DisplayName = "Dead")
};

UCLASS()
class SPARTA_H_API AHostageCharacter : public ACharacter, public IMissionInteractableInterface
{
	GENERATED_BODY()

public:
	AHostageCharacter();

	UPROPERTY(BlueprintReadOnly, Category = "Hostage")
	EHostageState CurrentState;

	UPROPERTY(BlueprintReadOnly, Category = "Hostage")
	AActor* TargetPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hostage")
	bool bIsSit = true; 
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hostage")
	bool bIsInteracted = false; 

protected:
	virtual void BeginPlay() override;

public:
	// --- [IMissionInteractableInterface 구현부] ---
	//뒤에 'override' 키워드를 붙여 부모 인터페이스의 함수를 재정의함을 명시합니다.
	virtual void Interact_Implementation(class APlayerCharacter* Interactor) override;
	virtual bool CanInteract_Implementation(class APlayerCharacter* Interactor) const override;
	virtual FString GetInteractionText_Implementation() const override;

	void ChangeState(EHostageState NewState);
	
	FTimerHandle StandUpTimerHandle;
	
	// 1. 플레이어 감지용 스피어 콜리전 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* DetectionSphere; // (주의: USphereComponent 입니다)

	// 2. 오버랩 이벤트 발생 시 실행될 함수
	UFUNCTION()
	void OnDetectionSphereOverlap(
		UPrimitiveComponent* OverlappedComponent, 
		AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, 
		int32 OtherBodyIndex, 
		bool bFromSweep, 
		const FHitResult& SweepResult
	);
	
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, 
							class AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	float Health = 100.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* DeathMontage;
};