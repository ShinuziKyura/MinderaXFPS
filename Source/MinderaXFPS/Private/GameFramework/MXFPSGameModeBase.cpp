// Ricardo Santos, 2023

#include "GameFramework/MXFPSGameModeBase.h"

#include <AIController.h>
#include <EngineUtils.h>
#include <GameFramework/Character.h>
#include <GameFramework/CharacterMovementComponent.h>
#include <GameFramework/GameUserSettings.h>
#include <GameFramework/PlayerStart.h>
#include <Kismet/GameplayStatics.h>
#include <NavMesh/RecastNavMesh.h>

namespace
{
#if (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT)
	FName const SaveGameName = TEXT("MXFPSSaveGame_Debug");
#else
	FName const SaveGameName = TEXT("MXFPSSaveGame_0");
#endif
	
	FName const UnknownSaveGameName = TEXT("SaveGame_Unknown");

	FIntPoint BestWindowedResolution = FIntPoint::ZeroValue;
	FIntPoint BestFullscreenResolution = FIntPoint(1280, 720);
}

AMXFPSGameModeBase::AMXFPSGameModeBase(FObjectInitializer const& ObjectInitializer)
	: Super{ ObjectInitializer }
	, GameOverResponsibleController{ nullptr }
	, PlayerScore{ 0 }
	, PlayerScoreOffset{ 0 }
	, PlayerSpawnLocation{ FVector::ZeroVector }
	, bIsGameRunning{ false }
	, bIsGamePaused{ false }
	, bShouldRestartGame{ false }
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AMXFPSGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	CacheBestResolutionPerWindowMode();

	UWorld* World = GetWorld();
	
	UClass* EnemyClass = EnemyClassPtr.Get();
	if (!ensureMsgf(EnemyClass, TEXT("GameMode was improperly configured: missing EnemyClassPtr!")))
	{
		return;
	}
	
	ARecastNavMesh* NavMesh = NavMeshPtr.Get();
	if (!ensureMsgf(NavMesh, TEXT("GameMode was improperly configured: missing NavMeshPtr!")))
	{
		return;
	}

	if (bModifySaveGame)
	{
		LoadSaveGame();
		
		if (bUseSaveGame)
		{
			NumEnemies = SaveGameData->NumEnemies;
			EnemySpeed = SaveGameData->EnemySpeed;
		}

		ChangeWindowMode(SaveGameData->bIsFullscreen ? EWindowMode::Fullscreen : EWindowMode::Windowed); 
	}

	float SafeRadiusSquared = FMath::Square(PlayerSafeRadius);
	for (int32 Index = 0; Index < NumEnemies; ++Index)
	{
		ACharacter* NewEnemy;
		FVector SpawnLocation;
		do
		{
			do
			{
				FNavLocation NavSpawnLocation;
				NavMesh->GetRandomReachablePointInRadius(LevelOrigin, LevelRadius, NavSpawnLocation);
			
				SpawnLocation = NavSpawnLocation.Location;
			}
			while (FVector::DistSquared(SpawnLocation, PlayerSpawnLocation) < SafeRadiusSquared);

			NewEnemy = World->SpawnActor<ACharacter>(EnemyClass, SpawnLocation, FRotator::ZeroRotator);
		}
		while (!NewEnemy);

		UCharacterMovementComponent* MovementComponent = NewEnemy->GetCharacterMovement();
		MovementComponent->MaxWalkSpeed = EnemySpeed * 100.f;
	}

	DisableAllInputAndMovement();

	GetWorldTimerManager().SetTimerForNextTick([this]{ OnNotifyGameReady(); });
}

void AMXFPSGameModeBase::EndPlay(EEndPlayReason::Type const EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (bModifySaveGame)
	{
		if (bUseSaveGame)
		{
			// If the exit was not requested by the player, check and update highscore
			if (HasNewHighscore() && GetGameOverResponsibleController() != GetWorld()->GetFirstPlayerController())
			{
				SaveGameData->HighScore = GetPlayerScore();
			}
		}
		
		StoreSaveGame();
	}
}

void AMXFPSGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsGameRunning)
	{
		auto GameCurrentTime = FDateTime::UtcNow();
		FTimespan ElapsedTime = GameCurrentTime - GameRunningTime;
	
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

	if (PlayerStart)
	{
		PlayerSpawnLocation = PlayerStart->GetActorLocation();
	}

	return PlayerStart;
}

void AMXFPSGameModeBase::OnNotifyGameReady_Implementation()
{
	ExecuteGameStart();
}

void AMXFPSGameModeBase::ExecuteGameStart()
{
	if (ensure(!bIsGameRunning))
	{
		bIsGameRunning = true;
		GameRunningTime = FDateTime::UtcNow();

		EnableAllInputAndMovement();
			
		OnGameStart.Broadcast();
	}
}

void AMXFPSGameModeBase::ExecuteGameOver(AController* ResponsibleController, bool bShouldRestart)
{
	if (ensure(bIsGameRunning))
	{
		check(ResponsibleController);
		
		bIsGameRunning = false;
		bShouldRestartGame = bShouldRestart;
		GameOverResponsibleController = ResponsibleController;

		DisableAllInputAndMovement();

		OnGameOver.Broadcast();
	}
}

void AMXFPSGameModeBase::ResumeGame()
{
	if (ensure(bIsGamePaused))
	{
		bIsGamePaused = false;
		GameRunningTime = FDateTime::UtcNow();

		EnableAllInputAndMovement();

		OnGameResumed.Broadcast();
	}
}

void AMXFPSGameModeBase::PauseGame()
{
	if (ensure(!bIsGamePaused))
	{
		bIsGamePaused = true;
		PlayerScoreOffset += PlayerScore;
		PlayerScore = 0;
		
		DisableAllInputAndMovement();

		OnGamePaused.Broadcast();
	}
}

void AMXFPSGameModeBase::ResetHighscore()
{
	if (bModifySaveGame)
	{
		SaveGameData->HighScore = -1;
	}
}

void AMXFPSGameModeBase::ChangeWindowMode(EWindowMode::Type NewWindowMode)
{
	FIntPoint NewResolution;
	switch (NewWindowMode)
	{
		case EWindowMode::Fullscreen:
			NewWindowMode = EWindowMode::WindowedFullscreen; 
		case EWindowMode::WindowedFullscreen:
			NewResolution = BestFullscreenResolution;
			break;
		case EWindowMode::Windowed:
			NewResolution = BestWindowedResolution;
			break;
		default:
			ensureMsgf(false, TEXT("Invalid WindowMode"));
			return;
	}

	auto GameUserSettings = UGameUserSettings::GetGameUserSettings();
	if (NewWindowMode != GameUserSettings->GetFullscreenMode())
	{	
		GameUserSettings->SetScreenResolution(NewResolution);
		GameUserSettings->SetFullscreenMode(NewWindowMode);
		GameUserSettings->ApplySettings(false);
	}
}

bool AMXFPSGameModeBase::IsGameRunning() const
{
	return bIsGameRunning && !bIsGamePaused;
}

bool AMXFPSGameModeBase::ShouldRestartGame() const
{
	return bShouldRestartGame;
}

bool AMXFPSGameModeBase::HasNewHighscore() const
{
	return GetPlayerScore() > GetPlayerHighscore();
}

int32 AMXFPSGameModeBase::GetPlayerScore() const
{
	return PlayerScore + PlayerScoreOffset;
}

int32 AMXFPSGameModeBase::GetPlayerHighscore() const
{
	return ensure(bModifySaveGame) ? SaveGameData->HighScore : -1;
}

AController* AMXFPSGameModeBase::GetGameOverResponsibleController() const
{
	return GameOverResponsibleController;
}

UMXFPSSaveGameData* AMXFPSGameModeBase::GetSaveGameData() const
{
	return SaveGameData;
}

void AMXFPSGameModeBase::RestartGame()
{
	GetWorld()->GetFirstPlayerController()->RestartLevel();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AMXFPSGameModeBase::EnableAllInputAndMovement() 
{
	UWorld* World = GetWorld();
	
	for (TActorIterator<APawn> ActorIter{ World }; ActorIter; ++ActorIter)
	{
		APawn* Pawn = *ActorIter;
		Pawn->EnableInput(nullptr);
		
		if (ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			AController* Controller = Pawn->GetController();
			bool bIsAIControlled = Controller ? Controller->IsA<AAIController>() : false;

			UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
			MovementComponent->SetMovementMode(bIsAIControlled ? MOVE_NavWalking : MOVE_Walking);
		}
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AMXFPSGameModeBase::DisableAllInputAndMovement()
{
	UWorld* World = GetWorld();
	
	for (TActorIterator<APawn> ActorIter{ World }; ActorIter; ++ActorIter)
	{
		APawn* Pawn = *ActorIter;
		Pawn->DisableInput(nullptr);
		
		if (ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
			MovementComponent->SetMovementMode(MOVE_None);
		}
	}
}

void AMXFPSGameModeBase::LoadSaveGame()
{
	if (auto SaveGame = UGameplayStatics::LoadGameFromSlot(SaveGameName.ToString(), 0))
	{
		SaveGameData = Cast<UMXFPSSaveGameData>(SaveGame);

		if (!ensureMsgf(SaveGameData, TEXT("SaveGame exists but has unknown class type!")))
		{
			UGameplayStatics::SaveGameToSlot(SaveGame, UnknownSaveGameName.ToString(), 0);
		}
	}

	if (!SaveGameData)
	{
		SaveGameData = NewObject<UMXFPSSaveGameData>(this, SaveGameName);
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AMXFPSGameModeBase::StoreSaveGame()
{
	UGameplayStatics::SaveGameToSlot(SaveGameData, SaveGameName.ToString(), 0);
}

void AMXFPSGameModeBase::CacheBestResolutionPerWindowMode()
{
	if (BestWindowedResolution == FIntPoint::ZeroValue)
	{
		TArray<FIntPoint> SupportedResolutions;
		if (UKismetSystemLibrary::GetSupportedFullscreenResolutions(SupportedResolutions))
		{
			for (auto Resolution : SupportedResolutions)
			{
				if (Resolution.X >= BestFullscreenResolution.X)
				{
					BestWindowedResolution = BestFullscreenResolution;
					BestFullscreenResolution = Resolution;
				}
			}
		}
		else
		{
			BestWindowedResolution = BestFullscreenResolution;
		}
	}
}
