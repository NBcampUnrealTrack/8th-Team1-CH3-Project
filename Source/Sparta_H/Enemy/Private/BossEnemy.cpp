#include "BossEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BossPrecisionWidget.h"
#include "Components/WidgetComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "ThrowableActor.h"
#include "CombatManager.h"
#include "BlackboardKeys.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ABossEnemy::ABossEnemy()
{
	PrecisionWarningWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("PrecisionWarningWidget"));
	PrecisionWarningWidgetComp->SetupAttachment(GetRootComponent());
	PrecisionWarningWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	PrecisionWarningWidgetComp->SetVisibility(false);
}

void ABossEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsAiming && PrecisionAimDuration > 0.f)
	{
		PrecisionAimElapsed += DeltaTime;
		const float Progress = FMath::Clamp(PrecisionAimElapsed / PrecisionAimDuration, 0.f, 1.f);
		if (UBossPrecisionWidget* W = GetPrecisionWidget())
		{
			W->UpdateWarningProgress(Progress);
		}
	}
}

void ABossEnemy::BeginPlay()
{
	Super::BeginPlay();

	AIPerceptionComp->OnTargetPerceptionUpdated.RemoveAll(this);
	AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &ABossEnemy::OnBossTargetPerceived);

	PrecisionWarningWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, PrecisionWidgetHeightOffset));
	PrecisionWarningWidgetComp->SetDrawSize(PrecisionWidgetDrawSize);

	if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
	{
		AnimInst->OnMontageEnded.AddDynamic(this, &ABossEnemy::OnSpecialMontageEnded);
		UE_LOG(LogTemp, Log, TEXT("[Boss] AnimInstance 바인딩 완료"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Boss] AnimInstance가 null — 몽타주 종료 델리게이트 바인딩 실패"));
	}

	UE_LOG(LogTemp, Log, TEXT("[Boss] BeginPlay 완료 | MaxHP=%.0f | Phase=%d"),
	       MaxHealth, (int32)CurrentPhase);
}

void ABossEnemy::InitializeStats()
{
	Super::InitializeStats(); // DA_Enemy_Boss → MaxHealth=500, WeaponDamage=35

	SightConfig->PeripheralVisionAngleDegrees = 180.f; // FOV 360도 (Half-angle)
	AIPerceptionComp->ConfigureSense(*SightConfig);
	AIPerceptionComp->RequestStimuliListenerUpdate();
}

// ─── Perception ──────────────────────────────────────────────────────────────
void ABossEnemy::OnBossTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
	if (bIsDead || !Actor || !Actor->ActorHasTag(TEXT("Player")))
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
			BossTarget = Actor;
			BB->SetValueAsObject(BBKeys::TARGET_ACTOR, Actor);
			BB->SetValueAsVector(BBKeys::LAST_KNOWN_LOCATION, Actor->GetActorLocation());

			if (CurrentAlertLevel != EAlertLevel::Combat)
			{
				UE_LOG(LogTemp, Log, TEXT("[Boss] 시야로 플레이어 발각 → Combat 진입"));
				OnAlertLevelChanged(EAlertLevel::Combat);
			}
		}
		else if (CurrentAlertLevel == EAlertLevel::Combat)
		{
			UE_LOG(LogTemp, Log, TEXT("[Boss] 플레이어 시야 이탈 → Lost"));
			OnAlertLevelChanged(EAlertLevel::Lost);
		}
	}
	else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			BB->SetValueAsVector(BBKeys::LAST_KNOWN_LOCATION, Stimulus.StimulusLocation);

			if (CurrentAlertLevel != EAlertLevel::Combat)
			{
				UE_LOG(LogTemp, Log, TEXT("[Boss] 청각으로 플레이어 감지 → Combat 진입"));
				BossTarget = Actor;
				BB->SetValueAsObject(BBKeys::TARGET_ACTOR, Actor);
				OnAlertLevelChanged(EAlertLevel::Combat);
			}
		}
	}
}

// ─── AlertLevel ──────────────────────────────────────────────────────────────
void ABossEnemy::OnAlertLevelChanged(EAlertLevel NewLevel)
{
	Super::OnAlertLevelChanged(NewLevel);

	if (NewLevel == EAlertLevel::Combat)
	{
		UE_LOG(LogTemp, Log, TEXT("[Boss] Combat 진입 — 버스트 사격 시작 | Phase=%d"), (int32)CurrentPhase);
		StartBurstCycle();

		if (CurrentPhase >= EBossPhase::Phase2 && !GetWorldTimerManager().IsTimerActive(GrenadeHandle))
		{
			UE_LOG(LogTemp, Log, TEXT("[Boss] Phase2+ 전투 재개 — 수류탄 타이머 복구"));
			GetWorldTimerManager().SetTimer(GrenadeHandle, this, &ABossEnemy::ThrowGrenade, GrenadeInterval, true, 3.f);
		}
		if (CurrentPhase == EBossPhase::Phase3 && !GetWorldTimerManager().IsTimerActive(PrecisionCycleHandle))
		{
			UE_LOG(LogTemp, Log, TEXT("[Boss] Phase3 전투 재개 — 정밀사격 타이머 복구"));
			StartPrecisionCycle();
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[Boss] 전투 해제 — 모든 공격 타이머 정지"));
		GetWorldTimerManager().ClearTimer(BurstCycleHandle);
		GetWorldTimerManager().ClearTimer(BurstShotHandle);
		GetWorldTimerManager().ClearTimer(GrenadeHandle);
		GetWorldTimerManager().ClearTimer(PrecisionCycleHandle);
		GetWorldTimerManager().ClearTimer(PrecisionFireHandle);
	}
}

// ─── TakeDamage / Die ────────────────────────────────────────────────────────
float ABossEnemy::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
                             AController* EventInstigator, AActor* DamageCauser)
{
	float Actual = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (!bIsDead)
	{
		UE_LOG(LogTemp, Log, TEXT("[Boss] 피격 — 데미지=%.0f | HP=%.0f/%.0f (%.0f%%) | Phase=%d"),
		       Actual, CurrentHealth, MaxHealth, (MaxHealth > 0.f ? CurrentHealth / MaxHealth * 100.f : 0.f),
		       (int32)CurrentPhase);
		TryPhaseTransition();
	}

	return Actual;
}

void ABossEnemy::Die()
{
	UE_LOG(LogTemp, Warning, TEXT("[Boss] ★ 보스 사망 — 모든 타이머 정지 + 보상 스폰"));
	GetWorldTimerManager().ClearTimer(BurstCycleHandle);
	GetWorldTimerManager().ClearTimer(BurstShotHandle);
	GetWorldTimerManager().ClearTimer(GrenadeHandle);
	GetWorldTimerManager().ClearTimer(PrecisionCycleHandle);
	GetWorldTimerManager().ClearTimer(PrecisionFireHandle);

	SpawnReward();
	Super::Die();
}

// ─── Phase ───────────────────────────────────────────────────────────────────
void ABossEnemy::TryPhaseTransition()
{
	if (MaxHealth <= 0.f)
	{
		return;
	}
	const float HPRatio = CurrentHealth / MaxHealth;

	if (CurrentPhase == EBossPhase::Phase1 && HPRatio <= Phase2HPRatio)
	{
		EnterPhase2();
	}
	else if (CurrentPhase == EBossPhase::Phase2 && HPRatio <= Phase3HPRatio)
	{
		EnterPhase3();
	}
}

void ABossEnemy::EnterPhase2()
{
	CurrentPhase = EBossPhase::Phase2;
	UE_LOG(LogTemp, Warning, TEXT("[Boss] ★ PHASE 2 진입 — 수류탄 + 증원 시작"));

	SpawnReinforcement();

	if (CurrentAlertLevel == EAlertLevel::Combat)
	{
		GetWorldTimerManager().SetTimer(GrenadeHandle, this, &ABossEnemy::ThrowGrenade, GrenadeInterval, true, 3.f);
		UE_LOG(LogTemp, Log, TEXT("[Boss] 수류탄 타이머 시작 | 간격=%.0f초"), GrenadeInterval);
	}
}

void ABossEnemy::EnterPhase3()
{
	CurrentPhase = EBossPhase::Phase3;
	UE_LOG(LogTemp, Warning, TEXT("[Boss] ★ PHASE 3 진입 — 이동속도 증가 + 정밀사격 시작"));

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed += 200.f;
		UE_LOG(LogTemp, Log, TEXT("[Boss] 이동속도 → %.0f"), MoveComp->MaxWalkSpeed);
	}

	if (CurrentAlertLevel == EAlertLevel::Combat || CurrentAlertLevel == EAlertLevel::Lost)
	{
		StartPrecisionCycle();
		UE_LOG(LogTemp, Log, TEXT("[Boss] 정밀사격 타이머 시작 | 간격=%.0f초"), PrecisionInterval);
	}
}

// ─── Burst Fire ──────────────────────────────────────────────────────────────
void ABossEnemy::StartBurstCycle()
{
	BurstShotCount = 0;
	ExecuteBurstStep();
}

void ABossEnemy::ExecuteBurstStep()
{
	if (bIsDead || CurrentAlertLevel != EAlertLevel::Combat || !IsValid(BossTarget))
	{
		BurstShotCount = 0;
		return;
	}

	if (bIsPerformingSpecialAttack)
	{
		UE_LOG(LogTemp, Log, TEXT("[Boss] 버스트 대기 — 특수 공격 중 (%.0f초 후 재시도)"), BurstInterval);
		GetWorldTimerManager().SetTimer(BurstCycleHandle, this, &ABossEnemy::StartBurstCycle, BurstInterval, false);
		return;
	}

	if (BurstShotCount < MaxBurstShots)
	{
		UE_LOG(LogTemp, Log, TEXT("[Boss] 버스트 사격 %d/%d"), BurstShotCount + 1, MaxBurstShots);
		if (CanShootTarget(BossTarget))
		{
			FireAtTarget(BossTarget);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[Boss] 버스트 — 타겟 사격 불가 (장애물 등)"));
		}
		BurstShotCount++;
		GetWorldTimerManager().SetTimer(BurstShotHandle, this, &ABossEnemy::ExecuteBurstStep, BurstShotInterval, false);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[Boss] 버스트 완료 — %.0f초 후 재시작"), BurstInterval);
		BurstShotCount = 0;
		GetWorldTimerManager().SetTimer(BurstCycleHandle, this, &ABossEnemy::StartBurstCycle, BurstInterval, false);
	}
}

// ─── Grenade ─────────────────────────────────────────────────────────────────
void ABossEnemy::ThrowGrenade()
{
	if (bIsDead || !GrenadeClass || !IsValid(BossTarget))
	{
		return;
	}

	bIsPerformingSpecialAttack = true;
	UE_LOG(LogTemp, Log, TEXT("[Boss] 수류탄 투척 시작"));

	if (GrenadeMontage)
	{
		PlayAnimMontage(GrenadeMontage);
		UE_LOG(LogTemp, Log, TEXT("[Boss] 수류탄 몽타주 재생"));
	}
	else
	{
		bIsPerformingSpecialAttack = false;
		UE_LOG(LogTemp, Warning, TEXT("[Boss] GrenadeMontage 미설정 — 즉시 플래그 해제"));
	}
	FTimerHandle ThrowTimerHandle;

	GetWorldTimerManager().SetTimer(
		ThrowTimerHandle,
		this,
		&ABossEnemy::SpawnGrenade,
		2.0f,
		false
	);
}


void ABossEnemy::SpawnGrenade()
{
	const FVector Origin = GetActorLocation() + FVector(0.f, 0.f, 100.f);
	const FVector TargetLocation = BossTarget->GetActorLocation();
	const FVector ThrowDir = (TargetLocation - Origin).GetSafeNormal();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this; // 보스가 던진 것임을 표시 → 플레이어에게 피해

	AThrowableActor* Grenade = GetWorld()->SpawnActor<AThrowableActor>(
		GrenadeClass, Origin, ThrowDir.Rotation(), SpawnParams);

	if (Grenade)
	{
		Grenade->ExplosionDamage = BossGrenadeDamage;
		Grenade->SetInstigator(this);
		Grenade->SetOwner(this);

		if (UPrimitiveComponent* GrenadeRoot = Cast<UPrimitiveComponent>(Grenade->GetRootComponent()))
		{
			GrenadeRoot->IgnoreActorWhenMoving(this, true);
			GetCapsuleComponent()->IgnoreActorWhenMoving(Grenade, true);
		}
		// SuggestProjectileVelocity_CustomArc으로 중력을 고려한 아크 궤적 계산
		FVector LaunchVelocity;
		const bool bFoundArc = UGameplayStatics::SuggestProjectileVelocity_CustomArc(
			this, LaunchVelocity, Origin, TargetLocation,
			0.f, // 기본 중력 사용
			0.4f // 아크 값: 0=직선, 1=고각도, 0.4=자연스러운 투척
		);

		if (bFoundArc)
		{
			// 계산 속도가 너무 느리면 GrenadeThrowSpeed로 스케일업 (방향은 유지)
			const float CalcSpeed = LaunchVelocity.Size();
			const float FinalSpeed = FMath::Max(CalcSpeed, GrenadeThrowSpeed);
			const FVector FinalVelocity = LaunchVelocity.GetSafeNormal() * FinalSpeed;
			Grenade->ProjectileMovement->Velocity = FinalVelocity;
			Grenade->ProjectileMovement->InitialSpeed = FinalSpeed;
			Grenade->ProjectileMovement->MaxSpeed = FinalSpeed;
			UE_LOG(LogTemp, Log, TEXT("[Boss] 수류탄 발사 (아크) | 속도=%.0f | 방향=(%.2f,%.2f,%.2f)"),
			       FinalSpeed, FinalVelocity.GetSafeNormal().X, FinalVelocity.GetSafeNormal().Y,
			       FinalVelocity.GetSafeNormal().Z);
		}
		else
		{
			Grenade->Launch(ThrowDir, GrenadeThrowSpeed);
			UE_LOG(LogTemp, Warning, TEXT("[Boss] 아크 계산 실패 — 직선 발사 | 방향=(%.2f,%.2f,%.2f)"),
			       ThrowDir.X, ThrowDir.Y, ThrowDir.Z);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Boss] 수류탄 스폰 실패"));
	}
}

// ─── Precision Shot ──────────────────────────────────────────────────────────
void ABossEnemy::StartPrecisionCycle()
{
	GetWorldTimerManager().SetTimer(PrecisionCycleHandle, this,
	                                &ABossEnemy::StartPrecisionAim, PrecisionInterval, true, PrecisionInterval);
}

UBossPrecisionWidget* ABossEnemy::GetPrecisionWidget() const
{
	if (!PrecisionWarningWidgetComp)
	{
		return nullptr;
	}
	return Cast<UBossPrecisionWidget>(PrecisionWarningWidgetComp->GetUserWidgetObject());
}

void ABossEnemy::StartPrecisionAim()
{
	if (bIsDead || CurrentAlertLevel != EAlertLevel::Combat || !IsValid(BossTarget))
	{
		return;
	}

	bIsPerformingSpecialAttack = true;
	UE_LOG(LogTemp, Warning, TEXT("[Boss] 정밀사격 조준 시작 — %.1f초 후 발사"), PrecisionAimDuration);

	if (PrecisionMontage)
	{
		PlayAnimMontage(PrecisionMontage);
		UE_LOG(LogTemp, Log, TEXT("[Boss] 정밀사격 몽타주 재생"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Boss] PrecisionMontage 미설정"));
	}

	if (PrecisionWarningWidgetComp)
	{
		PrecisionWarningWidgetComp->SetVisibility(true);
	}

	if (UBossPrecisionWidget* W = GetPrecisionWidget())
	{
		W->StartWarning();
		UE_LOG(LogTemp, Log, TEXT("[Boss] 경고 위젯 표시"));
	}
	else if (PrecisionWarningWidgetComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Boss] Widget 객체 없음 — WBP 클래스 확인 필요"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Boss] PrecisionWarningWidgetComp 자체가 null"));
	}

	PrecisionAimElapsed = 0.f;
	bIsAiming = true;
	GetWorldTimerManager().SetTimer(PrecisionFireHandle, this,
	                                &ABossEnemy::ExecutePrecisionFire, PrecisionAimDuration, false);
}

void ABossEnemy::ExecutePrecisionFire()
{
	bIsAiming = false;
	UE_LOG(LogTemp, Warning, TEXT("[Boss] 정밀사격 발사! | 데미지=%.0f"), PrecisionDamage);

	if (!PrecisionMontage)
	{
		bIsPerformingSpecialAttack = false;
	}

	if (bIsDead || !IsValid(BossTarget))
	{
		return;
	}

	if (UBossPrecisionWidget* W = GetPrecisionWidget())
	{
		W->StopWarning();
		UE_LOG(LogTemp, Log, TEXT("[Boss] 경고 위젯 숨김"));
	}

	if (PrecisionWarningWidgetComp)
	{
		PrecisionWarningWidgetComp->SetVisibility(false);
	}

	const FName MuzzleSocket = TEXT("Muzzle");
	const bool bHasMuzzle = WeaponMeshComp && WeaponMeshComp->DoesSocketExist(MuzzleSocket);
	const FVector AimStart = bHasMuzzle
		                         ? WeaponMeshComp->GetSocketLocation(MuzzleSocket)
		                         : GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);

	if (!bHasMuzzle)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Boss] Muzzle 소켓 없음 — 눈높이 위치에서 발사"));
	}

	// GetActorLocation()은 발 위치라 바닥에 맞으므로 흉부 높이를 향해 조준
	const FVector AimDir = (BossTarget->GetActorLocation() + FVector(0.f, 0.f, 80.f) - AimStart).GetSafeNormal();

	UE_LOG(LogTemp, Log, TEXT("[Boss] 정밀사격 발사 위치=(%.0f,%.0f,%.0f) | 방향=(%.2f,%.2f,%.2f)"),
	       AimStart.X, AimStart.Y, AimStart.Z, AimDir.X, AimDir.Y, AimDir.Z);

	// Modified: 정밀 사격 시에도 사격 사운드 재생 추가
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, AimStart);
	}

	CombatManagerComp->OnFire(AimStart, AimDir, ECombatWeaponType::Rifle, PrecisionDamage, true);
}

// ─── Reinforcement ───────────────────────────────────────────────────────────
void ABossEnemy::SpawnReinforcement()
{
	if (!ReinforcementClass || bHasSpawnedReinforcement)
	{
		return;
	}
	bHasSpawnedReinforcement = true;
	UE_LOG(LogTemp, Warning, TEXT("[Boss] 증원 스폰 시작 | 수=%d"), ReinforcementCount);

	FActorSpawnParameters SpawnParams;
	// Removed: 기존의 AlwaysSpawn 방식은 끼임 문제를 발생시킴 (삭제됨)
	// Modified: 끼임 방지를 위해 충돌 시 스폰하지 않도록 변경
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

	for (int32 i = 0; i < ReinforcementCount; ++i)
	{
		// 스폰 실패 시 재시도 로직 추가 및 Z축 오프셋 적용
		bool bSpawned = false;
		for (int32 Try = 0; Try < 5; ++Try)
		{
			const FVector DesiredLocation = GetActorLocation() + FVector(
				FMath::RandRange(-400.f, 400.f),
				FMath::RandRange(-400.f, 400.f),
				0.f);

			// NavMesh 위 유효한 위치로 보정
			FNavLocation NavLocation;
			UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
			FVector SpawnLocation = (NavSys && NavSys->ProjectPointToNavigation(
				                        DesiredLocation, NavLocation, FVector(200.f, 200.f, 200.f)))
				                        ? NavLocation.Location
				                        : DesiredLocation;

			// 바닥에 끼는 것을 방지하기 위해 약간 위쪽에서 스폰
			SpawnLocation.Z += 50.0f;

			if (!NavSys || NavLocation.Location == FVector::ZeroVector)
			{
				UE_LOG(LogTemp, Warning, TEXT("[Boss] 증원 %d번째 — NavMesh 위치 찾기 실패 (시도 %d)"), i + 1, Try + 1);
			}

			AEnemyCharacter* Reinforcement = GetWorld()->SpawnActor<AEnemyCharacter>(
				ReinforcementClass,
				SpawnLocation,
				FRotator::ZeroRotator,
				SpawnParams);

			if (Reinforcement)
			{
				bSpawned = true;
				if (!IsValid(BossTarget))
				{
					break;
				}

				UE_LOG(LogTemp, Log, TEXT("[Boss] 증원 %d번째 스폰 완료 (시도 %d)"), i + 1, Try + 1);
				Reinforcement->ShowExclamationIcon(3.f);

				// BT 초기화 완료 후 Combat 진입하도록 다음 틱으로 지연
				AActor* CapturedTarget = BossTarget;
				GetWorld()->GetTimerManager().SetTimerForNextTick([Reinforcement, CapturedTarget]()
				{
					if (!IsValid(Reinforcement) || !IsValid(CapturedTarget))
					{
						return;
					}

					AAIController* AIC = Cast<AAIController>(Reinforcement->GetController());
					if (!AIC)
					{
						UE_LOG(LogTemp, Error, TEXT("[Boss] 증원 — AIController 없음! BP에 AIController 클래스 확인 필요"));
						return;
					}

					UBehaviorTreeComponent* BTComp = AIC->FindComponentByClass<UBehaviorTreeComponent>();
					if (!BTComp || !BTComp->IsRunning())
					{
						UE_LOG(LogTemp, Error,
						       TEXT("[Boss] 증원 — BehaviorTree 실행 안 됨! BP_REnemyCharacter에 BT 애셋 할당 확인 필요"));
					}
					else
					{
						UE_LOG(LogTemp, Log, TEXT("[Boss] 증원 — BT 실행 중 확인"));
					}

					if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
					{
						BB->SetValueAsObject(BBKeys::TARGET_ACTOR, CapturedTarget);
						BB->SetValueAsVector(BBKeys::LAST_KNOWN_LOCATION, CapturedTarget->GetActorLocation());
					}

					Reinforcement->OnAlertLevelChanged(EAlertLevel::Combat);
					UE_LOG(LogTemp, Log, TEXT("[Boss] 증원 Combat 진입 완료 (지연 실행)"));
				});
				break;
			}
		}
	}
}

// ─── Fire Pattern Override ───────────────────────────────────────────────────
void ABossEnemy::StartFirePattern(AActor* TargetActor)
{
	// BT가 호출해도 무시 — 보스 사격은 BurstCycle에서 직접 관리
	UE_LOG(LogTemp, Log, TEXT("[Boss] StartFirePattern 차단 (BurstCycle이 대신 처리)"));
}

// ─── Debug ───────────────────────────────────────────────────────────────────
void ABossEnemy::Debug_ForcePhase2()
{
	if (CurrentPhase != EBossPhase::Phase1)
	{
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[Boss][Debug] Phase 2 강제 진입"));
	EnterPhase2();
}

void ABossEnemy::Debug_ForcePhase3()
{
	if (CurrentPhase == EBossPhase::Phase3)
	{
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[Boss][Debug] Phase 3 강제 진입"));
	if (CurrentPhase == EBossPhase::Phase1)
	{
		EnterPhase2();
	}
	EnterPhase3();
}

// ─── Montage End ─────────────────────────────────────────────────────────────
void ABossEnemy::OnSpecialMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == PrecisionMontage || Montage == GrenadeMontage)
	{
		bIsPerformingSpecialAttack = false;
		UE_LOG(LogTemp, Log, TEXT("[Boss] 특수 공격 몽타주 종료 (중단=%s) — 버스트 재개 가능"),
		       bInterrupted ? TEXT("O") : TEXT("X"));
	}
}

// ─── Reward ──────────────────────────────────────────────────────────────────
void ABossEnemy::SpawnReward()
{
	if (!RewardClass)
	{
		return;
	}

	GetWorld()->SpawnActor<AActor>(RewardClass, GetActorLocation(), FRotator::ZeroRotator);
}
