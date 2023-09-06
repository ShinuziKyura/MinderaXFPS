// Ricardo Santos, 2023

#include "GameFramework/MXFPSGameModeBase.h"

#include <AIController.h>
#include <EngineUtils.h>
#include <GameFramework/Character.h>
#include <GameFramework/CharacterMovementComponent.h>
#include <GameFramework/PlayerStart.h>
#include <NavMesh/RecastNavMesh.h>

AMXFPSGameModeBase::AMXFPSGameModeBase(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
	, PlayerScore{ 0 }
	, bIsGameRunning{ false }
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AMXFPSGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	
	UClass* EnemyClass = EnemyClassPtr.Get();
	checkf(EnemyClass, TEXT("GameMode was improperly configured: missing EnemyClassPtr!"));
	
	ARecastNavMesh* NavMesh = NavMeshPtr.Get();
	checkf(NavMesh, TEXT("GameMode was improperly configured: missing NavMeshPtr!"));

	float SafeRadiusSquared = FMath::Square(PlayerSafeRadius); 
	for (int32 Index = 0; Index < NumEnemies; ++Index)
	{
		FVector SpawnLocation;
		do
		{
			FNavLocation NavSpawnLocation;
			NavMesh->GetRandomReachablePointInRadius(LevelOrigin, LevelRadius, NavSpawnLocation);
			
			SpawnLocation = NavSpawnLocation.Location;
		}
		while (FVector::DistSquared(SpawnLocation, PlayerSpawnLocation) < SafeRadiusSquared);

		auto NewEnemy = World->SpawnActor<ACharacter>(EnemyClass, SpawnLocation, FRotator::ZeroRotator);

		auto MovementComponent = NewEnemy->GetCharacterMovement();
		MovementComponent->MaxWalkSpeed = EnemySpeed * 100.f;
	}

	DisableAllInputAndMovement();

	GetWorldTimerManager().SetTimerForNextTick([this]{ OnNotifyGameReady(); });
}

void AMXFPSGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsGameRunning)
	{
		auto GameCurrentTime = FDateTime::UtcNow();
		FTimespan ElapsedTime = GameCurrentTime - GameStartTime;
	
		PlayerScore = FMath::RoundToInt32(ElapsedTime.GetTotalSeconds());
	}
}

AActor* AMXFPSGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	AActor* PlayerStart;
	if (bUseRandomPlayerSpawn)
	{
		ARecastNavMesh* NavMesh = NavMeshPtr.LoadSynchronous();
		checkf(NavMesh, TEXT("GameMode was improperly configured: missing NavMeshPtr!"));
		
		FNavLocation NavLocation;
		NavMesh->GetRandomReachablePointInRadius(LevelOrigin, LevelRadius, NavLocation);

		FVector SpawnLocation = NavLocation.Location;
		FRotator SpawnRotation = FRotator{0.f, FMath::RandRange(0.f, 360.f), 0.f};

		PlayerStart = GetWorld()->SpawnActor<APlayerStart>(SpawnLocation, SpawnRotation);
	}
	else
	{
		PlayerStart = Super::ChoosePlayerStart_Implementation(Player);
	}

	PlayerSpawnLocation = PlayerStart->GetActorLocation();

	return PlayerStart;
}

void AMXFPSGameModeBase::OnNotifyGameReady_Implementation()
{
	ExecuteGameStart();
}

void AMXFPSGameModeBase::ExecuteGameStart()
{
	if (!bIsGameRunning)
	{
		bIsGameRunning = true;
		GameStartTime = FDateTime::UtcNow();

		EnableAllInputAndMovement();
			
		OnGameStart.Broadcast();
	}
}

void AMXFPSGameModeBase::ExecuteGameOver(APawn* ResponsibleActor)
{
	if (bIsGameRunning)
	{
		bIsGameRunning = false;
		GameOverResponsibleActor = ResponsibleActor;

		DisableAllInputAndMovement();
		
		OnGameOver.Broadcast();
	}
}

void AMXFPSGameModeBase::RestartGame()
{
	GetWorld()->GetFirstPlayerController()->RestartLevel();
}

bool AMXFPSGameModeBase::IsGameRunning() const
{
	return bIsGameRunning;
}

int32 AMXFPSGameModeBase::GetPlayerScore() const
{
	return PlayerScore;
}

APawn* AMXFPSGameModeBase::GetGameOverResponsibleActor() const
{
	return GameOverResponsibleActor;
}

void AMXFPSGameModeBase::EnableAllInputAndMovement() 
{
	auto World = GetWorld();
	
	for (TActorIterator<APawn> ActorIter{ World }; ActorIter; ++ActorIter)
	{
		APawn* Pawn = *ActorIter;
		Pawn->EnableInput(nullptr);
		
		if (ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			AController* Controller = Pawn->GetController();
			bool bIsAIControlled = Controller ? Controller->IsA<AAIController>() : false;

			auto MovementComponent = Character->GetCharacterMovement();
			MovementComponent->SetMovementMode(bIsAIControlled ? MOVE_NavWalking : MOVE_Walking);
		}
	}
}

void AMXFPSGameModeBase::DisableAllInputAndMovement()
{
	auto World = GetWorld();
	
	for (TActorIterator<APawn> ActorIter{ World }; ActorIter; ++ActorIter)
	{
		APawn* Pawn = *ActorIter;
		Pawn->DisableInput(nullptr);
		
		if (ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			auto MovementComponent = Character->GetCharacterMovement();
			MovementComponent->SetMovementMode(MOVE_None);
		}
	}
}
