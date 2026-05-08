#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponTypes.h"
#include "WeaponDataAsset.generated.h"

class UAnimInstance;
class UAnimMontage;
class USkeletalMesh;
class UNiagaraSystem;

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

	// 1인칭 팔(Mesh1P)에 재생할 발사 몽타주. 풀바디 임포트 애니는 팔 스켈레톤으로 리타게팅 후 몽타주화
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
	TSoftObjectPtr<UAnimMontage> FireMontage1P;

	// 1인칭 팔(Mesh1P)에 재생할 재장전 몽타주. 길이는 DA의 ReloadTime과 맞춰 세팅
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
	TSoftObjectPtr<UAnimMontage> ReloadMontage1P;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EWeaponType WeaponType = EWeaponType::Pistol;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EWeaponFireMode FireMode = EWeaponFireMode::SemiAuto;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float Damage = 0.0f;

	// 디테일 패널/UI/디버그에서 표시할 무기 이름. 로컬라이즈 대비 FText
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FText WeaponName;

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

	// 발사 시 무기 메시의 MuzzleSocketName 위치에 스폰할 총구 화염 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|VFX")
	TSoftObjectPtr<UNiagaraSystem> MuzzleFlashEffect;

	// 무기 스켈레탈 메시에 만들어 둔 총구 소켓 이름. 메시별로 다를 수 있어 DA에서 지정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|VFX")
	FName MuzzleSocketName = TEXT("Muzzle");
};
