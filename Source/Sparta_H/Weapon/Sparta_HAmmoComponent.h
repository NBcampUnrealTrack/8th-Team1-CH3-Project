#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sparta_HAmmoComponent.generated.h"

// 무기 인스턴스에 부착해 탄약 수를 관리하는 컴포넌트.
// 탄약이 있는 무기에만 부착하며, 근접/투척류는 사용하지 않음
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SPARTA_H_API USparta_HAmmoComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USparta_HAmmoComponent();

	// 무기 장착 시 DA의 MaxAmmoCount를 받아 초기 세팅
	UFUNCTION(BlueprintCallable, Category = "Ammo")
	void InitializeAmmo(int32 NewMaxAmmoCount);

	// 1발 소모. 탄이 없으면 false 반환 → 호출 측에서 발사 차단
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
