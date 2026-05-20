#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponTypes.h"
#include "WeaponDataAsset.generated.h"

class UTexture2D;
class AThrowableActor;
class AWeaponBase;

// 무기 한 종(개체가 아닌 종류)의 정적 스펙을 담는 DA.
// 런타임 상태(현재 탄약, 상태 등)는 캐릭터/컴포넌트에서 관리
UCLASS(BlueprintType)
class SPARTA_H_API UWeaponDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// ───── 식별 정보 ─────

	// 디테일 패널/UI/디버그에서 표시할 무기 이름. 로컬라이즈 대비 FText
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Identity")
	FText WeaponName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Identity")
	EWeaponType WeaponType = EWeaponType::Pistol;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Identity")
	EWeaponFireMode FireMode = EWeaponFireMode::SemiAuto;

	// ───── UI ─────

	// HUD/인벤토리 UI에서 표시할 무기 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|UI")
	TSoftObjectPtr<UTexture2D> WeaponIcon;

	// ───── 스탯 ─────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Stats")
	float Damage = 0.0f;

	// 거리 별 대미지 배율 (X: 거리(cm), Y: 배율(0.0~1.0))
	// 거리 별 대미지 배율 정보 추가
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Stats")
	UCurveFloat* DamageFalloffCurve;

	// 부위 별 대미지 배율
	// 부위 별 대미지 배율 정보 추가
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Stats")
	TMap<FName, float> BodyPartMultipliers;

	// 발사 간격(초). 풀오토에서 다음 발사까지의 쿨다운으로 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Stats")
	float FireRate = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Stats")
	int32 MaxAmmoCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Stats")
	float ReloadTime = 0.0f;

	// AI 청각 감지 반경(cm). 발사 시 이 범위 내 적이 어그로
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Stats")
	float SoundRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Stats")
	FRecoilData RecoilData;

	// ───── 동작 플래그 ─────

	// 재장전 가능 무기인지(근접/투척 무기는 false)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Behavior")
	bool bIsReloadable = false;

	// 탄창 소진 시 입력 없이 자동으로 재장전 시작할지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Behavior")
	bool bShouldAutoReload = false;

	// 발사 시 AI 어그로를 끌어야 하는 무기인지 (소음무기 등은 false)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Behavior")
	bool bShouldTriggerAIAggro = false;
	
	// 이 DA를 사용할 무기 BP 클래스. 메시/몽타주/VFX는 BP에서 직접 세팅
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Behavior")
	TSubclassOf<AWeaponBase> WeaponClass;

	// 투척 무기 전용 - FireMode가 Throwble인 무기에서 스폰할 액터 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Behavior")
	TSubclassOf<AThrowableActor> ThrowableClass;
	
	// ───── Throwable 전용 ─────
	
	// -1 이면 무한 (돌맹이용), 0 이상이면 제한된 보유 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Throwable", meta=(ClampMin = -1))
	int32 MaxStockCount = 3;
	
	// 던지고 다음 던지기까지(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Throwable")
	float ThrowCooldown = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Throwable")
	float MinThrowSpeed = 800.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Throwable")
	float MaxThrowSpeed = 2200.0f;
	
	// 0 ~ 100 차징 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Throwable")
	float ChargeDuration = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Throwable")
	float TrajectorySimTime = 3.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Throwable")
	float TrajectoryProjectileRadius = 5.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName AttachSocketName = FName(TEXT("GripPoint"));
	
};
