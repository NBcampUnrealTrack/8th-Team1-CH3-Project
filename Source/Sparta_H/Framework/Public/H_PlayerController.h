#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "H_PlayerController.generated.h"

class UH_HUDWidget;
class UInputMappingContext;
class UInputAction;

UCLASS()
class SPARTA_H_API AH_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AH_PlayerController();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputMappingContext* InputMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* MoveAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* JumpAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* LookAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* RunAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* HideAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* RollAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* LeanRightAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* LeanLeftAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* Interaction;

	// 무기 시스템 입력. 캐릭터에서 이 멤버를 참조해 BindAction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Weapon")
	UInputAction* FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Weapon")
	UInputAction* ReloadAction;

	// 1~3 키로 슬롯 직접 선택 (Axis1D, Scalar Modifier 1~3)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Weapon")
	UInputAction* EquipSlotAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Weapon")
	UInputAction* EquipNextWeaponAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Weapon")
	UInputAction* EquipPreviousWeaponAction;

	// G 키 — 투척물(수류탄/돌맹이) 장착 토글
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Weapon")
	UInputAction* ThrowableAction;

	// Tab 키 — 무기 목록 UI 표시
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|UI")
	UInputAction* ShowWeaponListAction;
	
	// 에디터에서 지정할 위젯 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UH_HUDWidget> HUDWidgetClass;

	// 무기 목록 위젯 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UH_WeaponListWidget> WeaponListWidgetClass;

	// 생성된 위젯 인스턴스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UH_HUDWidget> HUDWidgetInstance;

	// 무기 목록 위젯 인스턴스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<class UH_WeaponListWidget> WeaponListWidgetInstance;

	/** 미션 실패 위젯 클래스 **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UH_FailWidget> FailWidgetClass;

	/** 미션 실패 시 호출되는 함수 **/
	UFUNCTION(BlueprintCallable, Category = "Game")
	void ShowFailMenu(const FText& Reason);

	/** 게임 클리어 UI 클래스 **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UH_ClearWidget> ClearWidgetClass;

	/** 미션 클리어 시 호출되는 함수 **/
	UFUNCTION(BlueprintCallable, Category = "Game")
	void ShowClearMenu(float ClearTime, int32 KillCount);

	// UI 토글 처리 함수
	// Modified: 무기 목록 UI 토글 로직 및 상태 변수 추가
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleWeaponList();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	bool bIsWeaponListOpen = false;

protected:
	virtual void BeginPlay() override;

	// 입력 바인딩용 함수
	// Modified: 토글 전용 핸들러로 통합
	void OnToggleWeaponList();

	virtual void SetupInputComponent() override;
};
