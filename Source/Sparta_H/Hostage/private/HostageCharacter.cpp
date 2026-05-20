#include "Hostage/public/HostageCharacter.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "PlayerCharacter.h"

AHostageCharacter::AHostageCharacter()
{
    PrimaryActorTick.bCanEverTick = false;
    CurrentState = EHostageState::Stay;
    TargetPlayer = nullptr;

    if (GetCharacterMovement())
    {
       GetCharacterMovement()->bUseRVOAvoidance = true;
       GetCharacterMovement()->AvoidanceConsiderationRadius = 110.0f;
    }
}

void AHostageCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void AHostageCharacter::Interact_Implementation(APlayerCharacter* Interactor)
{
   if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("Hostage: 상호작용 실행됨!"));

   if (!CanInteract_Implementation(Interactor) || !Interactor) return;

   TargetPlayer = Interactor;

   if (CurrentState == EHostageState::Stay)
   {
      if (bIsSit == true)
      {
         // 애니메이션 블루프린트가 일어서는 모션을 재생하도록 스위치를 끕니다.
         bIsSit = false;
         
         // 4.83초 타이머를 가동하여 다 일어난 후에 Following 상태로 변경합니다.
         float StandUpDuration = 4.83f;
         GetWorld()->GetTimerManager().SetTimer(
            StandUpTimerHandle,
            [this]()
            {
               ChangeState(EHostageState::Following);
            },
            StandUpDuration,
            false
         );
         bIsInteracted = !bIsInteracted; // 상태 변경
      }
      
      // 2. 이미 서 있는 상황 (대기 상태에서 다시 따라오게 할 때)
      else
      {
         bIsInteracted = !bIsInteracted; // 상태 변경
         // 지연 시간(타이머) 없이 즉시 따라오도록 상태를 변경합니다.
         ChangeState(EHostageState::Following);
      }
   }
   else if (CurrentState == EHostageState::Following)
   {
      ChangeState(EHostageState::Stay);
      GetWorld()->GetTimerManager().ClearTimer(StandUpTimerHandle); 
      bIsInteracted = !bIsInteracted; 
   }
}

bool AHostageCharacter::CanInteract_Implementation(APlayerCharacter* Interactor) const
{
   return CurrentState != EHostageState::Dead;
}

FString AHostageCharacter::GetInteractionText_Implementation() const
{
   if (CurrentState == EHostageState::Stay)
   {
      return TEXT("따라오게 하기");
   }
   if (CurrentState == EHostageState::Following)
   {
      return TEXT("대기시키기");
   }
   return TEXT("");
}

void AHostageCharacter::ChangeState(EHostageState NewState)
{
   if (CurrentState == NewState) return;
   CurrentState = NewState;

   FString StateStr = (CurrentState == EHostageState::Following) ? TEXT("Following") : TEXT("Stay");
   if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Hostage: State -> %s"), *StateStr));

   if (AAIController* AIC = Cast<AAIController>(GetController()))
   {
      if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
      {
         BB->SetValueAsEnum(TEXT("EHostageState"), static_cast<uint8>(CurrentState));

         if (CurrentState == EHostageState::Following && TargetPlayer)
         {
            if (bIsSit) bIsSit = false;
            BB->SetValueAsObject(TEXT("TargetActor"), TargetPlayer); 
         }
         // ✨ [안전장치 추가] 다시 Stay 상태로 돌아왔을 때의 처리
         else if (CurrentState == EHostageState::Stay)
         {
            // 1. 블랙보드의 타겟 액터 정보를 깔끔하게 비워줍니다.
            BB->ClearValue(TEXT("TargetActor"));
            
            // 2. AI 컨트롤러에게 현재 실행 중인 모든 이동(MoveTo) 명령을 즉시 중지하라고 지시합니다.
            AIC->StopMovement();
         }
      }
   }
}