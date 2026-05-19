#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "H_ClearWidget.generated.h"

/**
 * 게임 클리어 시 표시되는 UI 위젯
 */
UCLASS()
class SPARTA_H_API UH_ClearWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 클리어 통계 데이터를 설정
	 * @param ClearTime 클리어에 걸린 시간 (초)
	 * @param KillCount 처치한 적의 수
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|Clear")
	void SetClearResult(float ClearTime, int32 KillCount);

protected:
	/** 클리어 시간 표시 텍스트 **/
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ClearTimeText;

	/** 처치 수 표시 텍스트 **/
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* KillCountText;

	/** 메인 메뉴 버튼 **/
	UPROPERTY(meta = (BindWidget))
	class UButton* MainMenuButton;

	/** 게임 종료 버튼 **/
	UPROPERTY(meta = (BindWidget))
	class UButton* QuitButton;

	virtual void NativeConstruct() override;

	/** 다시 시작 버튼 클릭 시 호출 **/
	UFUNCTION()
	void HandleOnMainMenuClicked();

	/** 게임 종료 버튼 클릭 시 호출 **/
	UFUNCTION()
	void HandleOnQuitClicked();
};
