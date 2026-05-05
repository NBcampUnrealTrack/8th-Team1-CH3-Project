#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sparta_HAmmoComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SPARTA_H_API USparta_HAmmoComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USparta_HAmmoComponent();

	UFUNCTION(BlueprintCallable, Category = "Ammo")
	void InitializeAmmo(int32 NewMaxAmmoCount);

	UFUNCTION(BlueprintCallable, Category = "Ammo")
	bool ConsumeAmmo();

	UFUNCTION(BlueprintCallable, Category = "Ammo")
	void ReloadAmmo();

	UFUNCTION(BlueprintCallable, Category = "Ammo")
	bool HasAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "Ammo")
	int32 GetCurrentAmmoCount() const;

	UFUNCTION(BlueprintCallable, Category = "Ammo")
	int32 GetMaxAmmoCount() const;

private:
	UPROPERTY(VisibleAnywhere, Category = "Ammo")
	int32 CurrentAmmoCount = 0;

	UPROPERTY(VisibleAnywhere, Category = "Ammo")
	int32 MaxAmmoCount = 0;
};
