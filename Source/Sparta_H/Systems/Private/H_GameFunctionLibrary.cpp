#include "Systems/Public/H_GameFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/Public/PlayerCharacter.h"
#include "Systems/Public/H_SaveGame.h"
#include "Enemy/Public/BaseEnemy.h"

// 정적 변수 초기화
bool UH_GameFunctionLibrary::bIsLoadPending = false;

void UH_GameFunctionLibrary::RequestLoadAndRespawn(const UObject* WorldContextObject)
{
	if (!WorldContextObject) return;

	// 1. 로드 예약 설정
	bIsLoadPending = true;

	// 2. 즉시 로드 시도
	// 인게임 로드라면 여기서 성공하여 플래그가 꺼질 것이고,
	// 레벨 이동 중이라면 플레이어를 못 찾아 플래그가 true인 상태로 유지됩니다.
	HandlePendingLoad(WorldContextObject, true);
	
	UE_LOG(LogTemp, Log, TEXT("Load and Respawn requested."));
}

void UH_GameFunctionLibrary::HandlePendingLoad(const UObject* WorldContextObject, bool bClearFlag)
{
	if (!bIsLoadPending || !WorldContextObject) return;

	const FString SAVE_SLOT = TEXT("SaveSlot");
	const int32 USER_INDEX = 0;

	if (UGameplayStatics::DoesSaveGameExist(SAVE_SLOT, USER_INDEX))
	{
		USaveGame* LoadedGame = UGameplayStatics::LoadGameFromSlot(SAVE_SLOT, USER_INDEX);
		if (UH_SaveGame* SaveData = Cast<UH_SaveGame>(LoadedGame))
		{
			APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(WorldContextObject, 0);
			if (APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(PlayerPawn))
			{
				// 체크포인트 데이터 구성
				FPlayerCheckpointData CheckpointData;
				CheckpointData.MissionIndex = SaveData->CurrentMission;
				CheckpointData.bHasRifle = SaveData->bHasRifle;
				CheckpointData.Location = SaveData->CurrentLocation;
				CheckpointData.Rotation = SaveData->CurrentRotation;
				CheckpointData.KillCount = SaveData->KillCount;
				CheckpointData.ElapsedTime = SaveData->SavedElapsedTime;

				// 플레이어 상태 복구
				PlayerChar->LoadCheckpoint(CheckpointData);

				// 기타 환경 정리
				ClearAllEnemies(WorldContextObject);
				UGameplayStatics::SetGamePaused(WorldContextObject, false);

				if (APlayerController* PC = Cast<APlayerController>(PlayerChar->GetController()))
				{
					FInputModeGameOnly InputMode;
					PC->SetInputMode(InputMode);
					PC->bShowMouseCursor = false;
				}

				// 실제로 로드에 성공했을 때만 플래그 해제
				if (bClearFlag)
				{
					bIsLoadPending = false;
				}
				UE_LOG(LogTemp, Log, TEXT("Load and Respawn applied successfully."));
			}
		}
	}
	else
	{
		// 세이브 파일 자체가 없으면 예약 취소
		bIsLoadPending = false;
		UE_LOG(LogTemp, Warning, TEXT("Save slot not found."));
	}
}

void UH_GameFunctionLibrary::RequestSaveGame(const UObject* WorldContextObject)
{
	const FString SAVE_SLOT = TEXT("SaveSlot");
	const int32 USER_INDEX = 0;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(WorldContextObject, 0);
	APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(PlayerPawn);
	
	if (!PlayerChar) return;

	USaveGame* SaveObject = UGameplayStatics::CreateSaveGameObject(UH_SaveGame::StaticClass());
	UH_SaveGame* SaveData = Cast<UH_SaveGame>(SaveObject);

	if (SaveData)
	{
		FPlayerCheckpointData Checkpoint = PlayerChar->SaveCheckpoint();

		SaveData->CurrentMission = Checkpoint.MissionIndex;
		SaveData->bHasRifle = Checkpoint.bHasRifle;
		SaveData->CurrentLocation = Checkpoint.Location;
		SaveData->CurrentRotation = Checkpoint.Rotation;
		SaveData->KillCount = Checkpoint.KillCount;
		SaveData->SavedElapsedTime = Checkpoint.ElapsedTime;

		UGameplayStatics::SaveGameToSlot(SaveData, SAVE_SLOT, USER_INDEX);
		UE_LOG(LogTemp, Log, TEXT("Game saved successfully."));
	}
}

void UH_GameFunctionLibrary::ClearAllEnemies(const UObject* WorldContextObject)
{
	if (!WorldContextObject) return;

	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsOfClass(WorldContextObject, ABaseEnemy::StaticClass(), FoundEnemies);

	for (AActor* Enemy : FoundEnemies)
	{
		if (IsValid(Enemy))
		{
			Enemy->Destroy();
		}
	}
}
