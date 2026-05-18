#include "Hostage/public/HostageCharacter.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AHostageCharacter::AHostageCharacter()
{
    PrimaryActorTick.bCanEverTick = false;
    CurrentState = EHostageState::Stay;
    TargetPlayer = nullptr;

    if (GetCharacterMovement())
    {
       GetCharacterMovement()->bUseRVOAvoidance = true;
    }
}

void AHostageCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
       TargetPlayer = PC->GetPawn();
    }
}

// C++ 입력 바인딩 활성화 (플레이어 컨트롤러의 키를 뺏어옴)
void AHostageCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (PlayerInputComponent)
    {
        PlayerInputComponent->BindKey(EKeys::F, IE_Pressed, this, &AHostageCharacter::OnFKeyPressed);
    }
}

void AHostageCharacter::OnFKeyPressed()
{
    if (TargetPlayer)
    {
        OnInteract(TargetPlayer);
    }
}

void AHostageCharacter::OnInteract(AActor* Interactor)
{
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("C++ : OnInteract Called!"));
    UE_LOG(LogTemp, Warning, TEXT("C++ : OnInteract Called! Interactor: %s"), Interactor ? *Interactor->GetName() : TEXT("None"));

    if (CurrentState == EHostageState::Dead || !Interactor) return;

    TargetPlayer = Interactor;

    if (CurrentState == EHostageState::Stay)
    {
       ChangeState(EHostageState::Following);
    }
    else if (CurrentState == EHostageState::Following)
    {
       ChangeState(EHostageState::Stay);
    }
}

void AHostageCharacter::ChangeState(EHostageState NewState)
{
    if (CurrentState == NewState) return;
    CurrentState = NewState;

    FString StateStr = (CurrentState == EHostageState::Following) ? TEXT("Following") : TEXT("Stay");
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("C++ : State Changed To -> %s"), *StateStr));
    UE_LOG(LogTemp, Warning, TEXT("C++ : State Changed To -> %s"), *StateStr);

    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
       if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
       {
          // 💡 [치트키 보정] AIController의 이름과 통일한 "EHostageState" 방에 
          // 0(Stay) 또는 1(Following) 이라는 명확한 순수 숫자로 변환하여 강제 주입합니다.
          BB->SetValueAsEnum(TEXT("EHostageState"), static_cast<uint8>(CurrentState));

          if (CurrentState == EHostageState::Following && TargetPlayer)
          {
             if (bIsSit) bIsSit = false;

             // 💡 TargetActor가 헷갈리신다고 하셨으니 확실하게 주입 로그를 한 번 더 검증합니다.
             BB->SetValueAsObject(TEXT("TargetActor"), TargetPlayer); 
             
             if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("C++ : TargetActor Set in Blackboard!"));
             UE_LOG(LogTemp, Warning, TEXT("C++ : TargetActor Set in Blackboard! Player: %s"), *TargetPlayer->GetName());
          }
       }
       else
       {
          UE_LOG(LogTemp, Error, TEXT("C++ Error : Blackboard Component is NULL!"));
       }
    }
    else
    {
       UE_LOG(LogTemp, Error, TEXT("C++ Error : AIController is NULL!"));
    }
}