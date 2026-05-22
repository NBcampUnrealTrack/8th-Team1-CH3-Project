#include "EnemyCharacter.h"
#include "DrawDebugHelpers.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "AlertManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/OverlapResult.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimMontage.h"
#include "BlackboardKeys.h"
#include "CombatManager.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

AEnemyCharacter::AEnemyCharacter()
{
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	CombatManagerComp = CreateDefaultSubobject<UCombatManager>(TEXT("CombatManager"));

	WeaponMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMeshComp->SetupAttachment(GetMesh(), TEXT("GripPoint"));

	AlertIconWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("AlertIconWidget"));
	AlertIconWidgetComp->SetupAttachment(GetRootComponent());
	AlertIconWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	AlertIconWidgetComp->SetDrawSize(FVector2D(64.f, 64.f));
	AlertIconWidgetComp->SetVisibility(false);

	if (SightConfig)
	{
		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		AIPerceptionComp->ConfigureSense(*SightConfig);
	}

	if (HearingConfig)
	{
		HearingConfig->HearingRange = IdleStats.HearingRange;
		HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
		HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
		HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
		AIPerceptionComp->ConfigureSense(*HearingConfig);
	}

	AIPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

	// 기본 상태에서는 Yaw 강제 고정을 끕니다. (MovementComponent가 회전을 제어하도록 위임)
	bUseControllerRotationYaw = false;
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (bUseRandomPatrol)
	{
		auto GetRandomOffset = []() -> FVector
		{
			float X = FMath::FRandRange(50.f, 150.f) * (FMath::RandBool() ? 1.f : -1.f);
			float Y = FMath::FRandRange(50.f, 150.f) * (FMath::RandBool() ? 1.f : -1.f);
			return FVector(X, Y, 0.f);
		};

		PatrolOffsetA = GetRandomOffset();
		PatrolOffsetB = GetRandomOffset();
	}

	PatrolSpawnLocation = GetActorLocation();

	AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyCharacter::OnTargetPerceived);
	ApplyPerceptionStats(IdleStats);

	AlertIconWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, AlertIconHeightOffset));

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bUseRVOAvoidance = true;
		// 캡슐 반경(~42)의 약 2.5배 — 접촉 전에 미리 회피 시작
		MoveComp->AvoidanceConsiderationRadius = 110.0f;
		MoveComp->bOrientRotationToMovement = true;
	}

	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			BB->SetValueAsVector(TEXT("PatrolLocationA"), GetPatrolWorldLocationA());
			BB->SetValueAsVector(TEXT("PatrolLocationB"), GetPatrolWorldLocationB());
		}
	}
}

void AEnemyCharacter::InitializeStats()
{
	Super::InitializeStats();
	WeaponDamage = Damage;
}

void AEnemyCharacter::ProcessSpotCheck()
{
	if (!SuspectedTarget)
	{
		GetWorldTimerManager().ClearTimer(SpotCheckTimerHandle);
		return;
	}

	float FinalChance = SpotProb;
	float RandomValue = FMath::FRand();

	if (FinalChance >= RandomValue)
	{
		OnAlertLevelChanged(EAlertLevel::Combat);
		GetWorldTimerManager().ClearTimer(SpotCheckTimerHandle);
		UE_LOG(LogTemp, Warning, TEXT("[%s] 플레이어 발각 확정! 타겟: %s"), *GetName(), *SuspectedTarget->GetName());
	}
	else
	{
		// 1초마다 0.15씩 증가 (약 2~3초 안에 확정 발견되도록)
		SpotProb += 0.15f;
	}
}

void AEnemyCharacter::UpdateAlertIcon(EAlertLevel NewLevel)
{
	if (!AlertIconWidgetComp)
	{
		return;
	}

	// Idle / CCTV / Lost : 즉시 숨김 + 타이머 취소
	if (NewLevel == EAlertLevel::Idle ||
		NewLevel == EAlertLevel::CCTV ||
		NewLevel == EAlertLevel::Lost)
	{
		GetWorldTimerManager().ClearTimer(IconHideTimerHandle);
		AlertIconWidgetComp->SetVisibility(false);
		return;
	}

	// Suspicious(??) / Combat(!!) : 아이콘 표시 후 IconHideDelay초 뒤 숨김
	UEnemyAlertWidget* AlertWidget = Cast<UEnemyAlertWidget>(AlertIconWidgetComp->GetUserWidgetObject());
	if (AlertWidget)
	{
		AlertWidget->OnAlertLevelUpdated(NewLevel);
	}

	AlertIconWidgetComp->SetVisibility(true);

	GetWorldTimerManager().SetTimer(
		IconHideTimerHandle, this,
		&AEnemyCharacter::HideAlertIcon,
		IconHideDelay, false
	);
}

void AEnemyCharacter::HideAlertIcon()
{
	if (AlertIconWidgetComp)
	{
		AlertIconWidgetComp->SetVisibility(false);
	}
}

void AEnemyCharacter::ShowExclamationIcon(float Duration)
{
	if (!AlertIconWidgetComp)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(IconHideTimerHandle);

	UEnemyAlertWidget* AlertWidget = Cast<UEnemyAlertWidget>(AlertIconWidgetComp->GetUserWidgetObject());
	if (AlertWidget)
	{
		AlertWidget->OnAlertLevelUpdated(EAlertLevel::Combat);
	}

	AlertIconWidgetComp->SetVisibility(true);
	GetWorldTimerManager().SetTimer(IconHideTimerHandle, this, &AEnemyCharacter::HideAlertIcon, Duration, false);
}

void AEnemyCharacter::ApplyPerceptionStats(const FAlertLevelStats& Stats)
{
	if (SightConfig)
	{
		SightConfig->SightRadius = Stats.SightRange;
		SightConfig->LoseSightRadius = Stats.SightRange;
		SightConfig->PeripheralVisionAngleDegrees = Stats.FOVAngle / 2.0f;
		AIPerceptionComp->ConfigureSense(*SightConfig);
	}

	if (HearingConfig)
	{
		HearingConfig->HearingRange = Stats.HearingRange;
		AIPerceptionComp->ConfigureSense(*HearingConfig);
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = Stats.MoveSpeed;
	}
	AIPerceptionComp->RequestStimuliListenerUpdate();
}

void AEnemyCharacter::OnAlertLevelChanged(EAlertLevel NewLevel)
{
	if (bIsDead)
	{
		return;
	}
	if (CurrentAlertLevel == NewLevel)
	{
		return;
	}

	// 1. C++ 내부 변수 업데이트
	CurrentAlertLevel = NewLevel;

	// 2. 블랙보드에 실시간 기록 (이게 없으면 BT가 작동 안 함)
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		// 경계 레벨이 변할 때 무조건 기존 이동을 강제 취소 (비헤이비어 트리가 잘못된 목적지로 가는 것 차단)
		AIC->StopMovement();

		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			BB->SetValueAsEnum(BBKeys::ALERT_LEVEL, static_cast<uint8>(NewLevel));
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[%s] 경계 레벨 변경: %d"), *GetName(), (int32)NewLevel);
	GetWorldTimerManager().ClearTimer(SuspiciousRevertTimerHandle);
	GetWorldTimerManager().ClearTimer(RepositionTimerHandle);
	bIsRepositioning = false;

	Super::OnAlertLevelChanged(NewLevel);

	AActor* TargetPlayer = nullptr;
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			TargetPlayer = Cast<AActor>(BB->GetValueAsObject(BBKeys::TARGET_ACTOR));
		}
	}

	bool bIsCombat = (NewLevel == EAlertLevel::Combat);

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		// Combat 상태일 때 항상 플레이어를 주시하도록 처리 (로스트 현상 방지)
		MoveComp->bOrientRotationToMovement = !bIsCombat;
		MoveComp->bUseControllerDesiredRotation = bIsCombat;

		if (bIsCombat)
		{
			// 몸을 빠르게 틀 수 있도록 회전 속도를 확 올려줍니다.
			MoveComp->RotationRate = FRotator(0.0f, 800.0f, 0.0f);
		}
	}

	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		// Combat 상태이고 타겟이 제대로 설정되어 있다면 확실하게 포커스 지정
		if (bIsCombat && TargetPlayer)
		{
			AIC->SetFocus(TargetPlayer, EAIFocusPriority::Gameplay);
		}
		else
		{
			AIC->ClearFocus(EAIFocusPriority::Gameplay);
		}
	}

	switch (NewLevel)
	{
	case EAlertLevel::Idle: ApplyPerceptionStats(IdleStats);
		break;
	case EAlertLevel::Suspicious: ApplyPerceptionStats(SuspiciousStats);
		break;
	case EAlertLevel::Combat: ApplyPerceptionStats(CombatStats);
		break;
	case EAlertLevel::Lost: ApplyPerceptionStats(LostStats);
		break;
	default: break;
	}

	switch (NewLevel)
	{
	case EAlertLevel::Suspicious:
		GetWorldTimerManager().SetTimer(SuspiciousRevertTimerHandle, this,
		                                &AEnemyCharacter::OnSuspiciousRevertTimerExpired, SuspiciousRevertDelay, false);
		break;
	case EAlertLevel::Combat:
		if (TargetPlayer)
		{
			AlertNearbyEnemies(TargetPlayer, CombatAlertRange, EAlertLevel::Combat);
		}
		if (AAlertManager* AlertMgr = AAlertManager::GetInstance(this))
		{
			AlertMgr->NotifyCombatEntered(GetActorLocation());
		}
		break;
	case EAlertLevel::Lost:
		if (AAIController* AIC = Cast<AAIController>(GetController()))
		{
			if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
			{
				BB->ClearValue(BBKeys::TARGET_ACTOR);
			}
		}
		if (TargetPlayer)
		{
			AlertNearbyEnemies(TargetPlayer, LostAlertRange, EAlertLevel::Suspicious);
		}
		break;
	default: break;
	}
}

// ---------------------------------------------------------------
// 주변 적 동기화
// ---------------------------------------------------------------
void AEnemyCharacter::AlertNearbyEnemies(AActor* TargetPlayer, float AlertRange, EAlertLevel NewLevel)
{
	if (!TargetPlayer)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	World->OverlapMultiByObjectType(
		OverlapResults,
		GetActorLocation(),
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(AlertRange),
		QueryParams
	);

	for (const FOverlapResult& Result : OverlapResults)
	{
		AEnemyCharacter* NearbyEnemy = Cast<AEnemyCharacter>(Result.GetActor());
		if (!NearbyEnemy || NearbyEnemy->IsDead())
		{
			continue;
		}
		if (NearbyEnemy->GetCurrentAlertLevel() >= NewLevel)
		{
			continue;
		}

		AAIController* AIC = Cast<AAIController>(NearbyEnemy->GetController());
		if (!AIC)
		{
			continue;
		}
		UBlackboardComponent* BB = AIC->GetBlackboardComponent();
		if (!BB)
		{
			continue;
		}

		// 블랙보드에 타겟 정보를 "먼저" 업데이트한 뒤에 OnAlertLevelChanged를 호출해야 합니다!
		BB->SetValueAsVector(BBKeys::LAST_KNOWN_LOCATION, TargetPlayer->GetActorLocation());
		if (NewLevel == EAlertLevel::Combat)
		{
			BB->SetValueAsObject(BBKeys::TARGET_ACTOR, TargetPlayer);
		}

		NearbyEnemy->OnAlertLevelChanged(NewLevel);
	}
}

void AEnemyCharacter::OnSuspiciousRevertTimerExpired()
{
	if (CurrentAlertLevel != EAlertLevel::Suspicious)
	{
		return;
	}

	OnAlertLevelChanged(EAlertLevel::Idle);

	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			BB->ClearValue(BBKeys::LAST_KNOWN_LOCATION);
			BB->ClearValue(BBKeys::TARGET_ACTOR);
		}
	}
}

void AEnemyCharacter::OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
	if (bIsDead || !Actor || Actor == this)
	{
		return;
	}
	if (!Actor->ActorHasTag(TEXT("Player")) || Actor->GetName().Contains(TEXT("Hostage")))
	{
		return;
	}

	AAIController* AIC = Cast<AAIController>(GetController());
	if (!AIC)
	{
		return;
	}
	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			SuspectedTarget = Actor;
			BB->SetValueAsObject(BBKeys::TARGET_ACTOR, Actor);
			BB->SetValueAsVector(BBKeys::LAST_KNOWN_LOCATION, Actor->GetActorLocation());

			GetWorldTimerManager().ClearTimer(CombatToLostTimerHandle);

			// 평상시(1단계)나 놓친(4단계) 상태일 때만 지켜보는 딜레이를 줌
			if (CurrentAlertLevel == EAlertLevel::Idle || CurrentAlertLevel == EAlertLevel::Lost)
			{
				if (CurrentAlertLevel == EAlertLevel::Lost)
				{
					UE_LOG(LogTemp, Warning, TEXT("[%s] 4단계(Lost) 상태에서 플레이어 포착! (확률 검사 시작)"), *GetName());
				}

				OnAlertLevelChanged(EAlertLevel::Suspicious);

				if (!GetWorldTimerManager().IsTimerActive(SpotCheckTimerHandle))
				{
					SpotProb = 0.7f;
					GetWorldTimerManager().SetTimer(SpotCheckTimerHandle, this, &AEnemyCharacter::ProcessSpotCheck,
					                                1.0f, true);
				}
			}
			// 수색(2단계) 중에 적이 눈앞에 나타나면, 딜레이 없이 즉각 전투(3단계) 개시!
			else if (CurrentAlertLevel == EAlertLevel::Suspicious)
			{
				GetWorldTimerManager().ClearTimer(SpotCheckTimerHandle);
				OnAlertLevelChanged(EAlertLevel::Combat);
			}
		}
		else
		{
			GetWorldTimerManager().ClearTimer(SpotCheckTimerHandle);

			if (CurrentAlertLevel == EAlertLevel::Combat)
			{
				GetWorldTimerManager().SetTimer(
					CombatToLostTimerHandle,
					this,
					&AEnemyCharacter::OnCombatToLostTimerExpired,
					5.0f,
					false
				);
			}
		}
	}
	else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			if (CurrentAlertLevel == EAlertLevel::Combat)
			{
				return;
			}

			UE_LOG(LogTemp, Warning, TEXT("[%s] 청각 감지! 소음 위치=%s | 현재 경계=%d"),
			       *GetName(), *Stimulus.StimulusLocation.ToString(), (int32)CurrentAlertLevel);

			BB->SetValueAsVector(BBKeys::LAST_KNOWN_LOCATION, Stimulus.StimulusLocation);

			if (CurrentAlertLevel == EAlertLevel::Idle)
			{
				AIC->StopMovement();
				OnAlertLevelChanged(EAlertLevel::Suspicious);
			}
			else if (CurrentAlertLevel == EAlertLevel::Suspicious)
			{
				AIC->StopMovement();
				GetWorldTimerManager().SetTimer(SuspiciousRevertTimerHandle, this,
				                                &AEnemyCharacter::OnSuspiciousRevertTimerExpired, SuspiciousRevertDelay,
				                                false);
			}
			else if (CurrentAlertLevel == EAlertLevel::Lost)
			{
				AIC->StopMovement();
				OnAlertLevelChanged(EAlertLevel::Suspicious);
				GetWorldTimerManager().SetTimer(SuspiciousRevertTimerHandle, this,
				                                &AEnemyCharacter::OnSuspiciousRevertTimerExpired, SuspiciousRevertDelay,
				                                false);
			}
		}
	}
}

void AEnemyCharacter::StartFirePattern(AActor* TargetActor)
{
	if (bIsDead || !TargetActor || TargetActor == this)
	{
		return;
	}
	if (TargetActor->GetName().Contains(TEXT("Hostage")))
	{
		return;
	}
	if (TargetActor->ActorHasTag(TEXT("Enemy")))
	{
		return;
	}

	if (GetWorldTimerManager().IsTimerActive(FirePatternTimerHandle))
	{
		return;
	}
	if (CurrentShotCount != 0)
	{
		return;
	}
	if (bIsRepositioning)
	{
		return;
	}

	SuspectedTarget = TargetActor;

	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		AIC->StopMovement();
		AIC->SetFocus(TargetActor, EAIFocusPriority::Gameplay);
	}

	UE_LOG(LogTemp, Warning, TEXT("[%s] 사격 패턴 시작! 타겟 액터: %s"), *GetName(), *TargetActor->GetName());

	ExecuteFireStep();
}

void AEnemyCharacter::ExecuteFireStep()
{
	FString TargetName = SuspectedTarget ? SuspectedTarget->GetName() : TEXT("None");

	if (bIsDead || CurrentAlertLevel != EAlertLevel::Combat || !SuspectedTarget
		|| SuspectedTarget->ActorHasTag(TEXT("Enemy")))
	{
		CurrentShotCount = 0;
		SuspectedTarget = nullptr;
		return;
	}

	if (CurrentShotCount < 3)
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] 점사 진행 중: 사격 조건을 검사합니다. 검사 대상: %s"), *GetName(), *TargetName);

		AActor* Blocker = nullptr;
		if (CanShootTarget(SuspectedTarget, &Blocker))
		{
			FireAtTarget(SuspectedTarget);
			CurrentShotCount++;

			UE_LOG(LogTemp, Log, TEXT("[%s] 발사 단계 성공. 0.4초 후 다음 탄 발사를 예약합니다. 대상: %s"), *GetName(), *TargetName);
			GetWorldTimerManager().SetTimer(FirePatternTimerHandle, this, &AEnemyCharacter::ExecuteFireStep, 0.4f,
			                                false);
		}
		else
		{
			const FVector DirToTarget = (SuspectedTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
			const float AngleToTarget = FMath::RadiansToDegrees(
				FMath::Acos(FVector::DotProduct(GetActorForwardVector(), DirToTarget)));

			if (AngleToTarget > FireAngleLimit)
			{
				UE_LOG(LogTemp, Log, TEXT("[%s] 타겟이 시야각(%.1f도) 밖에 있어 사격 보류 및 몸 회전 대기 중..."), *GetName(), AngleToTarget);
				GetWorldTimerManager().SetTimer(FirePatternTimerHandle, this, &AEnemyCharacter::ExecuteFireStep, 0.1f,
				                                false);
			}
			else if (Blocker && Blocker->ActorHasTag(TEXT("Enemy")))
			{
				UE_LOG(LogTemp, Warning, TEXT("[%s] 아군(%s)에 막혀 재배치 시작"), *GetName(), *Blocker->GetName());
				CurrentShotCount = 0;
				bIsRepositioning = true;

				if (AAIController* AIC = Cast<AAIController>(GetController()))
				{
					AIC->MoveToActor(SuspectedTarget, 600.f);
				}

				GetWorldTimerManager().SetTimer(RepositionTimerHandle, this, &AEnemyCharacter::TryRepositionForShot,
				                                0.5f, false);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[%s] 점사 도중 사격 조건을 상실하여 사격을 종료합니다. 대상: %s"), *GetName(), *TargetName);
				CurrentShotCount = 0;
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] 3점사 발사 완료. 0.8초간 사격 쿨다운 제어에 들어갑니다. 대상: %s"), *GetName(), *TargetName);
		CurrentShotCount = 0;
		GetWorldTimerManager().SetTimer(FirePatternTimerHandle, 0.8f, false);
	}
}

void AEnemyCharacter::TryRepositionForShot()
{
	if (bIsDead || CurrentAlertLevel != EAlertLevel::Combat || !SuspectedTarget)
	{
		bIsRepositioning = false;
		return;
	}

	AActor* Blocker = nullptr;
	if (CanShootTarget(SuspectedTarget, &Blocker))
	{
		bIsRepositioning = false;

		if (AAIController* AIC = Cast<AAIController>(GetController()))
		{
			AIC->StopMovement();
		}

		UE_LOG(LogTemp, Warning, TEXT("[%s] 시야 확보 완료 — 사격 재개"), *GetName());
		ExecuteFireStep();
	}
	else if (Blocker && Blocker->ActorHasTag(TEXT("Enemy")))
	{
		if (AAIController* AIC = Cast<AAIController>(GetController()))
		{
			AIC->MoveToActor(SuspectedTarget, 600.f);
		}

		GetWorldTimerManager().SetTimer(RepositionTimerHandle, this, &AEnemyCharacter::TryRepositionForShot, 0.5f,
		                                false);
	}
	else
	{
		bIsRepositioning = false;
	}
}

void AEnemyCharacter::OnDetectionTimerExpired()
{
	if (bIsDead || !SuspectedTarget)
	{
		return;
	}

	AAIController* AIC = Cast<AAIController>(GetController());
	if (!AIC)
	{
		return;
	}
	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	BB->SetValueAsObject(BBKeys::TARGET_ACTOR, SuspectedTarget);
	OnAlertLevelChanged(EAlertLevel::Combat);
	SuspectedTarget = nullptr;
}

void AEnemyCharacter::Die()
{
	Super::Die();

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetWorldTimerManager().ClearTimer(RepositionTimerHandle);
	bIsRepositioning = false;

	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		AIC->StopMovement();
	}
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
	}
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		AIC->UnPossess();
	}

	UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (DeathMontage && AnimInst)
	{
		const float MontageDuration = AnimInst->Montage_Play(DeathMontage, 1.0f);
		const float DestroyDelay = FMath::Max(MontageDuration - 0.2f, 0.1f);

		FTimerHandle DestroyTimerHandle;
		GetWorldTimerManager().SetTimer(DestroyTimerHandle, [this]() { Destroy(); }, DestroyDelay, false);
	}
	else
	{
		Destroy();
	}
}

// ---------------------------------------------------------------
// 사격 가시성 및 트레이스 디버그 라인 표현
// ---------------------------------------------------------------
bool AEnemyCharacter::CanShootTarget(AActor* TargetActor, AActor** OutBlocker)
{
	if (!TargetActor)
	{
		return false;
	}

	if (OutBlocker)
	{
		*OutBlocker = nullptr;
	}

	// 1. 거리 제한 검사
	const float Distance = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	if (Distance > FireRange)
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] CanShoot FAIL — 거리 초과 (%.0f / %.0f)"), *GetName(), Distance, FireRange);
		return false;
	}

	// 2. 시야각 제한 검사
	const FVector DirectionToTarget = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	const float AngleToTarget = FMath::RadiansToDegrees(
		FMath::Acos(FVector::DotProduct(GetActorForwardVector(), DirectionToTarget))
	);
	if (AngleToTarget > FireAngleLimit)
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] CanShoot FAIL — 각도 초과 (%.1f° / %.1f°)"), *GetName(), AngleToTarget,
		       FireAngleLimit);
		return false;
	}

	// 3. 라인 트레이스 세팅 (자기 자신만 무시 — 아군 적은 사격선을 막을 수 있음)
	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);
	if (WeaponMeshComp)
	{
		CollisionParams.AddIgnoredComponent(WeaponMeshComp);
	}
	if (GetCapsuleComponent())
	{
		CollisionParams.AddIgnoredComponent(GetCapsuleComponent());
	}

	// 4. 발사 시작 지점(총구)
	const FName MuzzleSocket = TEXT("Muzzle");
	FVector StartLocation = (WeaponMeshComp && WeaponMeshComp->DoesSocketExist(MuzzleSocket))
		                        ? WeaponMeshComp->GetSocketLocation(MuzzleSocket)
		                        : GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);

	// 5. 조준점(타겟) 보정: 플레이어 캡슐 정중앙으로 계산
	FVector TargetLocation = TargetActor->GetActorLocation();
	if (ACharacter* TargetChar = Cast<ACharacter>(TargetActor))
	{
		TargetLocation += FVector(0.f, 0.f, TargetChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.7f);
	}
	else
	{
		TargetLocation += FVector(0.f, 0.f, 80.f);
	}

	// 6. ECC_Pawn 채널로 트레이스 (아군 적 캡슐 감지)
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult, StartLocation, TargetLocation,
		ECC_Pawn,
		CollisionParams
	);

	// 7. 디버그 라인: 초록색(성공), 빨간색(충돌)
	DrawDebugLine(GetWorld(), StartLocation, bHit ? HitResult.ImpactPoint : TargetLocation,
	              bHit ? FColor::Red : FColor::Green, false, 2.0f, 0, 2.0f);

	// 8. 명중 판정
	bool bCanShoot = bHit && (HitResult.GetActor() == TargetActor);

	if (!bCanShoot && OutBlocker)
	{
		*OutBlocker = HitResult.GetActor();
	}

	UE_LOG(LogTemp, Log, TEXT("[%s] CanShootTarget 결과: %d | 적중 액터: %s"),
	       *GetName(), bCanShoot, HitResult.GetActor() ? *HitResult.GetActor()->GetName() : TEXT("None"));

	return bCanShoot;
}

bool AEnemyCharacter::FireAtTarget(AActor* TargetActor)
{
	if (bIsDead || !TargetActor || !CombatManagerComp)
	{
		return false;
	}

	const FName MuzzleSocket = TEXT("Muzzle");
	FVector AimStart = (WeaponMeshComp && WeaponMeshComp->DoesSocketExist(MuzzleSocket))
		                   ? WeaponMeshComp->GetSocketLocation(MuzzleSocket)
		                   : GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);

	FVector FinalTargetLocation = TargetActor->GetActorLocation();
	if (ACharacter* TargetChar = Cast<ACharacter>(TargetActor))
	{
		FinalTargetLocation.Z += TargetChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.7f;
	}
	else
	{
		FinalTargetLocation.Z += 80.f;
	}

	const FVector AimDirection = (FinalTargetLocation - AimStart).GetSafeNormal();

	if (FireMontage)
	{
		PlayAnimMontage(FireMontage);
	}
	if (MuzzleFlashEffect && WeaponMeshComp)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(MuzzleFlashEffect, WeaponMeshComp, TEXT("Muzzle"),
		                                             FVector::ZeroVector, FRotator::ZeroRotator,
		                                             EAttachLocation::SnapToTarget, true);
	}
	// Modified: 사격 사운드를 총구 위치(AimStart)에서 재생하도록 변경
	if (FireSound)
	{
		// Removed: 기존 캐릭터 위치 재생 방식 제거
		// UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, AimStart);
	}

	float ShootProb = FMath::FRand();
	// 70>60프로로 하향, 대미지 9->6 하향
	if (ShootProb <= 0.6f)
	{
		CombatManagerComp->OnFire(AimStart, AimDirection, ECombatWeaponType::Rifle, WeaponDamage, true);
	}
	return true;
}

void AEnemyCharacter::OnCombatToLostTimerExpired()
{
	if (bIsDead || CurrentAlertLevel != EAlertLevel::Combat)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[%s] 플레이어를 5초간 놓쳐 4단계(Lost) 상태로 전환합니다."), *GetName());

	SuspectedTarget = nullptr;

	OnAlertLevelChanged(EAlertLevel::Lost);
}

void AEnemyCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (GetWorld() && GetWorld()->WorldType == EWorldType::Editor)
	{
		FVector Origin = GetActorLocation();
		DrawDebugSphere(GetWorld(), Origin + PatrolOffsetA, 30.f, 12, FColor::Green, false, -1.f, 0, 2.f);
		DrawDebugSphere(GetWorld(), Origin + PatrolOffsetB, 30.f, 12, FColor::Red, false, -1.f, 0, 2.f);
	}
}
