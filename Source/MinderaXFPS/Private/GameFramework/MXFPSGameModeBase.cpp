// Ricardo Santos, 2023

#include "GameFramework/MXFPSGameModeBase.h"

#include <AIController.h>
#include <GameFramework/Character.h>
#include <GameFramework/CharacterMovementComponent.h>
#include <GameFramework/PlayerStart.h>
#include <NavMesh/RecastNavMesh.h>

AMXFPSGameModeBase::AMXFPSGameModeBase(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AMXFPSGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	
	UClass* EnemyClass = EnemyClassPtr.Get();
	checkf(EnemyClass, TEXT("GameMode was improperly configured: missing EnemyClassPtr!"));
	
	ARecastNavMesh* NavMesh = NavMeshPtr.Get();
	checkf(NavMesh, TEXT("GameMode was improperly configured: missing NavMeshPtr!"));

	for (int32 Index = 0; Index < NumEnemies; ++Index)
	{
		FNavLocation SpawnLocation;
		NavMesh->GetRandomReachablePointInRadius(LevelOrigin, LevelRadius, SpawnLocation);

		auto NewEnemy = World->SpawnActor<ACharacter>(EnemyClass, SpawnLocation.Location, FRotator::ZeroRotator);

		auto MovementComponent = NewEnemy->GetCharacterMovement();
		MovementComponent->MaxWalkSpeed = EnemySpeed * 100.f; 
	}

	GameStartTime = FDateTime::UtcNow();
}

void AMXFPSGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	auto GameCurrentTime = FDateTime::UtcNow();
	FTimespan ElapsedTime = GameCurrentTime - GameStartTime;
	
	CurrentScore = FMath::RoundToInt32(ElapsedTime.GetTotalSeconds());
}

AActor* AMXFPSGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	if (bUseRandomPlayerSpawn)
	{
		ARecastNavMesh* NavMesh = NavMeshPtr.LoadSynchronous();
		checkf(NavMesh, TEXT("GameMode was improperly configured: missing NavMeshPtr!"));
		
		FNavLocation SpawnLocation;
		NavMesh->GetRandomReachablePointInRadius(LevelOrigin, LevelRadius, SpawnLocation);

		return GetWorld()->SpawnActor<APlayerStart>(SpawnLocation.Location,
													FRotator(0.f, FMath::RandRange(0.f, 360.f), 0.f));
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

int32 AMXFPSGameModeBase::GetCurrentScore() const
{
	return CurrentScore;
}

void AMXFPSGameModeBase::NotifyPlayerCaptured(AActor* Player, AActor* Enemy)
{
	auto GameFinishTime = FDateTime::UtcNow();
	FTimespan ElapsedTime = GameFinishTime - GameStartTime;

	int32 FinalScore = FMath::RoundToInt32(ElapsedTime.GetTotalSeconds());

	// TODO logic to finish and restart the game
}
