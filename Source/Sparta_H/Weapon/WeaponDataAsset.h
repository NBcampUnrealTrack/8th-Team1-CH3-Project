#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponTypes.h"
#include "WeaponDataAsset.generated.h"

class UAnimInstance;
class USkeletalMesh;

// 무기 한 종(개체가 아닌 종류)의 정적 스펙을 담는 DA.
// 런타임 상태(현재 탄약, 상태 등)는 캐릭터/컴포넌트에서 관리
UCLASS(BlueprintType)
class SPARTA_H_API UWeaponDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 1인칭에서 GripPoint 소켓에 부착할 무기 메시. 패키징 용량을 위해 SoftObjectPtr 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSoftObjectPtr<USkeletalMesh> WeaponMesh;

	// 무기 메시 자체의 AnimBP (팔 AnimBP와는 별개). 슬라이드/볼트 같은 무기 측 애니용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSoftClassPtr<UAnimInstance> WeaponAnimationClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EWeaponType WeaponType = EWeaponType::Pistol;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EWeaponFireMode FireMode = EWeaponFireMode::SemiAuto;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float Damage = 0.0f;

	// 백어택 보너스 데미지. 적 후방 판정 시 Damage 대신 적용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float BackAttackDamage = 0.0f;

	// AI 청각 감지 반경(cm). 발사 시 이 범위 내 적이 어그로
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float SoundRange = 0.0f;

	// 발사 간격(초). 풀오토에서 다음 발사까지의 쿨다운으로 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float FireRate = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 MaxAmmoCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float ReloadTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FRecoilData RecoilData;

	// 재장전 가능 무기인지(근접/투척 무기는 false)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bIsReloadable = false;

	// 탄창 소진 시 입력 없이 자동으로 재장전 시작할지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bShouldAutoReload = false;

	// 발사 시 AI 어그로를 끌어야 하는 무기인지 (소음무기 등은 false)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bShouldTriggerAIAggro = false;
};
