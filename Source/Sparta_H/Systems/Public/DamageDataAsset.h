#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DamageDataAsset.generated.h"

// 거리-데미지 배율 한 구간을 표현하는 구조체
USTRUCT(BlueprintType)
struct FDistanceDamageEntry
{ 
	GENERATED_BODY()

	// 이 구간의 최대 거리(cm). 예: 1000 = 10m
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	float MaxDistance = 0.f;

	// 이 구간에서 적용할 데미지 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	float DamageMultiplier = 1.f;
};

// 무기 종류별(주무기/권총/적 등) 데미지 거리 감쇠 테이블을 담는 DataAsset
UCLASS(BlueprintType)
class SPARTA_H_API UDamageDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 거리 구간 배열. MaxDistance 오름차순으로 정렬해서 입력할 것.
	// 마지막 구간 초과 시 배율 0.0(무효) 처리됨.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	TArray<FDistanceDamageEntry> DistanceTable;

	// 이 데이터 에셋이 어떤 무기/액터 종류인지 레이블 (에디터 식별용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	FString Label = TEXT("Default");

	// 거리에 따른 배율 반환. 구간 초과 시 0.0f 반환
	float GetMultiplierByDistance(float Distance) const
	{
		for (const FDistanceDamageEntry& Entry : DistanceTable)
		{
			if (Distance <= Entry.MaxDistance)
			{
				UE_LOG(LogTemp, Warning, TEXT("[DamageDataAsset][%s] Distance: %.1fcm → Matched MaxDistance: %.1fcm, Multiplier: %.2f"),
					*Label, Distance, Entry.MaxDistance, Entry.DamageMultiplier);
				return Entry.DamageMultiplier;
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("[DamageDataAsset][%s] Distance: %.1fcm → 최대 사거리 초과, Multiplier: 0.0"),
			*Label, Distance);
		return 0.f; // 최대 사거리 초과
	}
};