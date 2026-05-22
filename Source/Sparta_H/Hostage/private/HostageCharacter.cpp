#include "Hostage/public/HostageCharacter.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "BrainComponent.h"

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
   
   DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
   if (DetectionSphere)
   {
      DetectionSphere->SetupAttachment(GetRootComponent());
      DetectionSphere->SetSphereRadius(200.0f); // 반지름 원하는 크기로 조절 가능
        
      // 콜리전 프로필 설정 (플레이어만 감지하도록 쿼리전용 설정)
      DetectionSphere->SetCollisionProfileName(TEXT("Trigger")); 
  }
}

void AHostageCharacter::BeginPlay()
{
    Super::BeginPlay();
   
   if (DetectionSphere)
   {
      DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &AHostageCharacter::OnDetectionSphereOverlap);
   }
}

void AHostageCharacter::OnDetectionSphereOverlap(
    UPrimitiveComponent* OverlappedComponent, 
    AActor* OtherActor, 
    UPrimitiveComponent* OtherComp, 
    int32 OtherBodyIndex, 
    bool bFromSweep, 
    const FHitResult& SweepResult)
{
   if (CurrentState == EHostageState::Dead) return;
   
   APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
   if (Player)
   {
      if (CurrentState == EHostageState::Following)
      {
         if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("Hostage: 플레이어와 접촉하여 대기(Stay) 상태로 변경됩니다!"));
         
         GetWorld()->GetTimerManager().ClearTimer(StandUpTimerHandle); 
         
         bIsInteracted = false;
         ChangeState(EHostageState::Stay);
      }
   }
}

void AHostageCharacter::Interact_Implementation(APlayerCharacter* Interactor)
{
   if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("Hostage: 상호작용 실행됨!"));

   if (!CanInteract_Implementation(Interactor) || !Interactor) return;

   TargetPlayer = Interactor;
   
   if (GetWorld()->GetTimerManager().IsTimerActive(StandUpTimerHandle))
   {
      if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Hostage: 일어나는 중입니다. 기다려주세요!"));
      return; 
   }

   if (CurrentState == EHostageState::Stay)
   {
      if (bIsSit == true)
      {
         bIsSit = false;
         bIsInteracted = true;
         
         // 4.83초 타이머를 가동하여 다 일어난 후에 Following 상태로 변경합니다.
         float StandUpDuration = 4.83f;
         GetWorld()->GetTimerManager().SetTimer(
            StandUpTimerHandle,
            [this]()
            {
               ChangeState(EHostageState::Following);
               StandUpTimerHandle.Invalidate();
            },
            StandUpDuration,
            false
         );
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
      bIsInteracted = false;
      GetWorld()->GetTimerManager().ClearTimer(StandUpTimerHandle); 
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
         else if (CurrentState == EHostageState::Dead)
         {
            // 1. AI 및 이동 로직 완전 정지
            AIC->UnPossess(); // 컨트롤러와 폰의 연결 해제 (브레인 정지 포함)
            
            // 2. 물리 및 콜리전 설정
            GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            GetCharacterMovement()->DisableMovement();

            // 3. 사망 몽타주 재생 및 연결 해제 전략
            if (DeathMontage)
            {
               // 몽타주 재생
               
               // [핵심] 일정 시간(몽타주 길이) 후에 AnimBP 연결을 끊고 애니메이션을 중지시킵니다.
               FTimerHandle TimerHandle;
               GetWorldTimerManager().SetTimer(TimerHandle, [this]()
               {
                   // 애니메이션 블루프린트와의 연결을 완전히 끊음
                   GetMesh()->SetAnimInstanceClass(nullptr); 
                
                   // 현재 포즈에서 고정하고 더 이상 애니메이션 계산(Tick)을 하지 않음
                   GetMesh()->bPauseAnims = true;
                   GetMesh()->SetComponentTickEnabled(false);
                  
                   GetWorldTimerManager().ClearAllTimersForObject(this);
                
                   UE_LOG(LogTemp, Log, TEXT("인질: 애니메이션 시스템 연결이 해제되었습니다."));
               }, 1.6, false);
            }
         }
      }
   }
}

float AHostageCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, 
                                    AController* EventInstigator, AActor* DamageCauser)
{
   if (CurrentState == EHostageState::Dead) return 0.f;

   const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
   
   Health -= ActualDamage;
   if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Hostage 피격! 남은 체력: %.1f"), Health));

   if (Health <= 0.f)
   {
      // 4.83초 기상 타이머나 일시정지 타이머가 돌고 있었다면 안전하게 해제
      GetWorld()->GetTimerManager().ClearTimer(StandUpTimerHandle);
      StandUpTimerHandle.Invalidate();

      // 사망 상태로 변경
      ChangeState(EHostageState::Dead);
      
      if (DeathMontage)
      {
         PlayAnimMontage(DeathMontage);
      }
      
      if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Black, TEXT("Hostage: 인질이 사망했습니다."));
   }

   return ActualDamage;
}