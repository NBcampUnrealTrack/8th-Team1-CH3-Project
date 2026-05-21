#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "H_GameFunctionLibrary.generated.h"

/**
 * 게임 전반에 걸친 유틸리티 함수들을 제공하는 클래스
 */
UCLASS()
class SPARTA_H_API UH_GameFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	 // 세이브 데이터를 로드하고 플레이어 상태를 복구하며 적들을 제거
	UFUNCTION(BlueprintCallable, Category = "Game", meta = (WorldContext = "WorldContextObject"))
	static void RequestLoadAndRespawn(const UObject* WorldContextObject);

	/** 현재 플레이어의 상태를 "SaveSlot"에 저장 **/
	UFUNCTION(BlueprintCallable, Category = "Game", meta = (WorldContext = "WorldContextObject"))
	static void RequestSaveGame(const UObject* WorldContextObject);

	/** 맵에 있는 모든 적을 찾아 제거 **/
	UFUNCTION(BlueprintCallable, Category = "Game", meta = (WorldContext = "WorldContextObject"))
	static void ClearAllEnemies(const UObject* WorldContextObject);

	// 모든 스폰 트리거 및 볼륨을 초기 상태로 되돌림
	UFUNCTION(BlueprintCallable, Category = "Game", meta = (WorldContext = "WorldContextObject"))
	static void ResetAllSpawnSystems(const UObject* WorldContextObject);

	//레벨 전환 후 로드가 필요한지 확인하고 적용하는 함수
	static void HandlePendingLoad(const UObject* WorldContextObject, bool bClearFlag = true);

private:
	// 로드 요청 상태를 저장하는 정적 변수
	static bool bIsLoadPending;
};
