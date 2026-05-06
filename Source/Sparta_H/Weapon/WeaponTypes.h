#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.h"
#include "WeaponTypes.generated.h"

// 무기 종류. AnimBP State Machine 분기 및 DA 분류 키로 사용
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	Knife,
	Pistol,
	Rifle,
	Rock
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
	Swapping
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
	Default, // 일반
	ADS, // 정조준
	KillConfirm // 적 처치 시
};


// 무기 시스템 enum -> 컴뱃 시스템 enum 매핑.
// 두 enum은 별도라 변환이 필요 (통합 리팩터는 별도 작업)
inline ECombatWeaponType ToCombatWeaponType(EWeaponType Type)
{
	switch (Type)
	{
	case EWeaponType::Knife:  return ECombatWeaponType::Knife;
	case EWeaponType::Pistol: return ECombatWeaponType::Pistol;
	case EWeaponType::Rifle:  return ECombatWeaponType::Rifle;
	case EWeaponType::Rock:   return ECombatWeaponType::Rock;
	default:                  return ECombatWeaponType::None;
	}
}