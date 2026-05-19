#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.h"
#include "WeaponTypes.generated.h"

// 무기 종류. AnimBP State Machine 분기 및 DA 분류 키로 사용.
// ECombatWeaponType과 동일 순서/값으로 미러링 — static_cast로 직접 변환 가능
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	Rifle,
	Pistol,
	Knife,
	Grenade,
	Rock,
	None
};

// 발사 방식. 트리거 입력 처리 로직(연사/단발/투척)을 결정
UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
	Melee,
	SemiAuto,
	FullAuto,
	Throwable
};

// 무기의 현재 동작 상태. 장착/발사/재장전 중 입력 가드용
UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	Idle,
	Firing,
	Reloading,
	Swapping,
	ChargingThrow
};

// 반동 파라미터. DA에서 무기별로 세팅
USTRUCT(BlueprintType)
struct FRecoilData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil")
	float VerticalRecoil = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil")
	float HorizontalRecoil = 0.0f;

	// 반동이 적용되는 속도를 결정하는 필드
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil")
	float RecoilSpeed = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil")
	float RecoverySpeed = 0.0f;
};

// 인벤토리 슬롯 한 칸의 런타임 상태(타입 + 상태) 표현용
USTRUCT(BlueprintType)
struct FWeaponSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EWeaponType WeaponType = EWeaponType::Pistol;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EWeaponState WeaponState = EWeaponState::Idle;
};

// 크로스헤어의 시각적 상태 정의
UENUM(BlueprintType)
enum class ECrosshairState : uint8
{
	Default,     // 일반
	Pistol,      // 권총
	Rifle,       // 라이플
	KillConfirm  // 적 처치 시
};


// 무기 시스템 enum -> 컴뱃 시스템 enum 매핑.
// 두 enum은 동일 순서/값으로 미러링되어 있어 static_cast로 변환 가능.
// 한쪽 enum 값을 추가/제거할 경우 반드시 다른 쪽도 같은 순서로 맞출 것
inline ECombatWeaponType ToCombatWeaponType(EWeaponType Type)
{
	return static_cast<ECombatWeaponType>(Type);
}
