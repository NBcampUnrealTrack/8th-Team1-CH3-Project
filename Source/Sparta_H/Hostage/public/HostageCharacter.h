#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HostageCharacter.generated.h"

UENUM(BlueprintType)
enum class EHostageState : uint8
{
	Stay       = 0  UMETA(DisplayName = "Stay"),
	Following  = 1  UMETA(DisplayName = "Following"),
	Dead       = 2  UMETA(DisplayName = "Dead")
};

UCLASS()
class SPARTA_H_API AHostageCharacter : public ACharacter
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

protected:
	virtual void BeginPlay() override;

public:
	// 키보드 입력을 가로채기 위한 내장 함수
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    
	UFUNCTION(BlueprintCallable, Category = "Hostage")
	void OnInteract(AActor* Interactor);

	void ChangeState(EHostageState NewState);

private:
	void OnFKeyPressed();
};