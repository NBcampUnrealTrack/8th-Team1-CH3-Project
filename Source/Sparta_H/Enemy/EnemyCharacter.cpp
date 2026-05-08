#include "EnemyCharacter.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "TimerManager.h"

AEnemyCharacter::AEnemyCharacter()
{
	// Perception 컴포넌트 및 기본 감지 에셋 생성
	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));

	if (SightConfig)
	{
		SightConfig->SightRadius = SightRange;
		SightConfig->LoseSightRadius = SightRange + 300.0f;
		SightConfig->PeripheralVisionAngleDegrees = VisualFOV / 2.0f; // 반각 기준
		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

		AIPerceptionComp->ConfigureSense(*SightConfig);
	}

	if (HearingConfig)
	{
		HearingConfig->HearingRange = HearingRange;
		HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
		HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
		HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;

		AIPerceptionComp->ConfigureSense(*HearingConfig);
	}

	AIPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyCharacter::OnTargetPerceived);
}

void AEnemyCharacter::OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
	if (bIsDead) return;

	AAIController* AIC = Cast<AAIController>(GetController());
	if (!AIC) return;
	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	if (!BB) return;

	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			if (!GetWorldTimerManager().IsTimerActive(DetectionTimerHandle) && CurrentAlertLevel < EAlertLevel::Combat)
			{
				SuspectedTarget = Actor;

				GetWorldTimerManager().SetTimer(
					DetectionTimerHandle,
					this,
					&AEnemyCharacter::OnDetectionTimerExpired,
					1.0f,
					false
				);

				BB->SetValueAsVector(TEXT("LastKnownLocation"), Stimulus.StimulusLocation);
			}
		}
		else
		{
			if (GetWorldTimerManager().IsTimerActive(DetectionTimerHandle))
			{
				GetWorldTimerManager().ClearTimer(DetectionTimerHandle);
				SuspectedTarget = nullptr;
			}

			if (CurrentAlertLevel == EAlertLevel::Combat)
			{
				OnAlertLevelChanged(EAlertLevel::Lost);
			}
		}
	}
	else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
	{
		if (CurrentAlertLevel < EAlertLevel::Suspicious && Stimulus.WasSuccessfullySensed())
		{
			OnAlertLevelChanged(EAlertLevel::Suspicious);
			BB->SetValueAsVector(TEXT("LastKnownLocation"), Stimulus.StimulusLocation);
		}
	}
}

// 1초 동안 성공적으로 시야를 잃지 않고 버텼을 때 호출되는 발각 처리 함수
void AEnemyCharacter::OnDetectionTimerExpired()
{
	if (bIsDead || !SuspectedTarget) return;

	AAIController* AIC = Cast<AAIController>(GetController());
	if (!AIC) return;
	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	if (!BB) return;

	OnAlertLevelChanged(EAlertLevel::Combat);
	BB->SetValueAsObject(TEXT("TargetActor"), SuspectedTarget);

	SuspectedTarget = nullptr;
}

bool AEnemyCharacter::CanShootTarget(AActor* TargetActor)
{
	if (!TargetActor) return false;

	float Distance = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	if (Distance > FireRange) return false;

	FVector DirectionToTarget = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	float AngleToTarget = FMath::RadiansToDegrees(
		FMath::Acos(FVector::DotProduct(GetActorForwardVector(), DirectionToTarget)));

	if (AngleToTarget > FireAngleLimit) return false;

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);

	FVector StartLocation = GetMesh()->GetSocketLocation(TEXT("MuzzleSocket"));
	if (StartLocation.IsZero())
	{
		StartLocation = GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
	}

	FVector EndLocation = TargetActor->GetActorLocation();

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECC_Visibility,
		CollisionParams
	);

	if (bHit)
	{
		if (HitResult.GetActor() == TargetActor)
		{
			return true;
		}
	}

	return false;
}
