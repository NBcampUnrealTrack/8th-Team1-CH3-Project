#include "Systems/Public/H_GameFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/Public/PlayerCharacter.h"
#include "Systems/Public/H_SaveGame.h"
#include "Enemy/Public/BaseEnemy.h"
#include "Spawn/Public/RespawnTrigger.h"
#include "Spawn/Public/BaseSpawnVolume.h"

// 정적 변수 초기화
bool UH_GameFunctionLibrary::bIsLoadPending = false;

void UH_GameFunctionLibrary::RequestLoadAndRespawn(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return;
	}

	// 로드 요청 플래그를 설정하고 즉시 시도. 
	// 레벨 이동 시에는 타이머가 취소되어 플래그가 유지되고, 이동이 없으면 다음 프레임에 플래그가 해제됨.
	bIsLoadPending = true;
	HandlePendingLoad(WorldContextObject, false);

	if (UWorld* World = WorldContextObject->GetWorld())
	{
		FTimerHandle TempHandle;
		World->GetTimerManager().SetTimerForNextTick([]()
		{
			bIsLoadPending = false;
		});
	}

	UE_LOG(LogTemp, Log, TEXT("Load and Respawn requested."));
}

void UH_GameFunctionLibrary::HandlePendingLoad(const UObject* WorldContextObject, bool bClearFlag)
{
	if (!bIsLoadPending || !WorldContextObject)
	{
		return;
	}

	const FString SAVE_SLOT = TEXT("SaveSlot");
	constexpr int32 USER_INDEX = 0;

	// 1. 세이브 데이터 존재 여부 확인
	if (UGameplayStatics::DoesSaveGameExist(SAVE_SLOT, USER_INDEX))
	{
		// 2. 데이터 로드 및 캐스팅
		USaveGame* LoadedGame = UGameplayStatics::LoadGameFromSlot(SAVE_SLOT, USER_INDEX);
		if (UH_SaveGame* SaveData = Cast<UH_SaveGame>(LoadedGame))
		{
			// 3. 플레이어 캐릭터 찾아오기
			APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(WorldContextObject, 0);
			if (APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(PlayerPawn))
			{
				// 4. 로드된 데이터를 기반으로 체크포인트 데이터 생성
				FPlayerCheckpointData CheckpointData;
				CheckpointData.MissionIndex = SaveData->CurrentMission;
				CheckpointData.bHasRifle = SaveData->bHasRifle;
				CheckpointData.Location = SaveData->CurrentLocation;
				CheckpointData.Rotation = SaveData->CurrentRotation;
				CheckpointData.KillCount = SaveData->KillCount;
				CheckpointData.ElapsedTime = SaveData->SavedElapsedTime;

				// 5. 플레이어에게 로드 요청
				PlayerChar->LoadCheckpoint(CheckpointData);

				// 6. 맵상의 모든 적 제거
				ClearAllEnemies(WorldContextObject);

				// Modified: 모든 스폰 시스템(트리거 및 볼륨) 상태 초기화
				ResetAllSpawnSystems(WorldContextObject);

				// 7. 게임 일시정지 해제 및 입력 모드 복구
				UGameplayStatics::SetGamePaused(WorldContextObject, false);

				if (APlayerController* PC = Cast<APlayerController>(PlayerChar->GetController()))
				{
					FInputModeGameOnly InputMode;
					PC->SetInputMode(InputMode);
					PC->bShowMouseCursor = false;
				}

				// 성공 시 호출자에 따라 플래그 초기화 여부 결정
				if (bClearFlag)
				{
					bIsLoadPending = false;
				}
				UE_LOG(LogTemp, Log, TEXT("Load and Respawn applied."));
			}
		}
	}
	else
	{
		bIsLoadPending = false;
		UE_LOG(LogTemp, Warning, TEXT("Save slot not found."));
	}
}

void UH_GameFunctionLibrary::RequestSaveGame(const UObject* WorldContextObject)
{
	const FString SAVE_SLOT = TEXT("SaveSlot");
	constexpr int32 USER_INDEX = 0;

	// 1. 플레이어 캐릭터 찾아오기
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(WorldContextObject, 0);
	APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(PlayerPawn);

	if (!PlayerChar)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to find PlayerCharacter for saving."));
		return;
	}

	// 2. 새로운 세이브 객체 생성 (또는 기존 로드)
	USaveGame* SaveObject = UGameplayStatics::CreateSaveGameObject(UH_SaveGame::StaticClass());
	UH_SaveGame* SaveData = Cast<UH_SaveGame>(SaveObject);

	if (SaveData)
	{
		// 3. 캐릭터로부터 현재 체크포인트 데이터 획득
		FPlayerCheckpointData Checkpoint = PlayerChar->SaveCheckpoint();

		// 4. 세이브 데이터 필드에 값 복사
		SaveData->CurrentMission = Checkpoint.MissionIndex;
		SaveData->bHasRifle = Checkpoint.bHasRifle;
		SaveData->CurrentLocation = Checkpoint.Location;
		SaveData->CurrentRotation = Checkpoint.Rotation;
		SaveData->KillCount = Checkpoint.KillCount;
		SaveData->SavedElapsedTime = Checkpoint.ElapsedTime;

		// 5. 슬롯에 저장
		if (UGameplayStatics::SaveGameToSlot(SaveData, SAVE_SLOT, USER_INDEX))
		{
			UE_LOG(LogTemp, Log, TEXT("Game saved successfully to %s"), *SAVE_SLOT);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save game to %s"), *SAVE_SLOT);
		}
	}
}

void UH_GameFunctionLibrary::ClearAllEnemies(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return;
	}

	TArray<AActor*> FoundEnemies;
	// 모든 ABaseEnemy 상속 클래스의 액터를 찾습니다.
	UGameplayStatics::GetAllActorsOfClass(WorldContextObject, ABaseEnemy::StaticClass(), FoundEnemies);

	static const FName CCTV_TAG = FName("CCTV");

	for (AActor* Enemy : FoundEnemies)
	{
		if (IsValid(Enemy) && !Enemy->ActorHasTag(CCTV_TAG))
		{
			Enemy->Destroy();
		}
	}
}

// Modified: 모든 스폰 트리거와 볼륨 상태를 초기화하는 구현체 수정 (순서 보정: 볼륨 -> 트리거)
void UH_GameFunctionLibrary::ResetAllSpawnSystems(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return;
	}

	// 1. 모든 스폰 볼륨을 먼저 Idle 상태로 초기화
	TArray<AActor*> FoundVolumes;
	UGameplayStatics::GetAllActorsOfClass(WorldContextObject, ABaseSpawnVolume::StaticClass(), FoundVolumes);
	for (AActor* Actor : FoundVolumes)
	{
		if (ABaseSpawnVolume* Volume = Cast<ABaseSpawnVolume>(Actor))
		{
			Volume->ResetVolume();
		}
	}

	// 2. 그 다음 리스폰 트리거를 초기화 (플레이어가 안에 있다면 여기서 위에서 초기화된 볼륨을 다시 Active로 만듦)
	TArray<AActor*> FoundTriggers;
	UGameplayStatics::GetAllActorsOfClass(WorldContextObject, ARespawnTrigger::StaticClass(), FoundTriggers);
	for (AActor* Actor : FoundTriggers)
	{
		if (ARespawnTrigger* Trigger = Cast<ARespawnTrigger>(Actor))
		{
			Trigger->ResetTrigger();
		}
	}
}
