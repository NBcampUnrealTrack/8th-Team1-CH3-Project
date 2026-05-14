#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "H_FailWidget.generated.h"

UCLASS()
class SPARTA_H_API UH_FailWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 실패 원인 텍스트를 설정
	 * @param FailReason 실패 원인 (인질 사망, 시간 초과, 플레이어 사망)
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|Fail")
	void SetFailReasonText(const FText& FailReason);

protected:
	/** 블루프린트에서 바인딩할 실패 사유 텍스트 위젯 **/
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* FailReasonText;

	/** 다시 시작 버튼 **/
	UPROPERTY(meta = (BindWidget))
	class UButton* RestartButton;

	/** 메인 메뉴 버튼 (선택 사항) **/
	UPROPERTY(meta = (BindWidget))
	class UButton* MainMenuButton;

	virtual void NativeConstruct() override;

	/** 다시 시작 버튼 클릭 시 호출 **/
	UFUNCTION()
	void HandleOnRestartClicked();

	/** 메인 메뉴 버튼 클릭 시 호출 **/
	UFUNCTION()
	void HandleOnMainMenuClicked();
};
