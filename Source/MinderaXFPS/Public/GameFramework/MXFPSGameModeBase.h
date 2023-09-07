// Ricardo Santos, 2023

#pragma once

#include <CoreMinimal.h>
#include <GameFramework/GameModeBase.h>
#include <GameFramework/SaveGame.h>

#include "MXFPSGameModeBase.generated.h"

UCLASS(BlueprintType)
class UMXFPSSaveGameData : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	int32 NumEnemies = 4;

	UPROPERTY(BlueprintReadWrite)
	int32 EnemySpeed = 5;

	UPROPERTY(BlueprintReadWrite)
	int32 HighScore = -1;

	UPROPERTY(BlueprintReadWrite)
	bool bIsFullscreen = true;
};

UCLASS(Blueprintable)
class MINDERAXFPS_API AMXFPSGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	explicit AMXFPSGameModeBase(FObjectInitializer const& ObjectInitializer);

	virtual void BeginPlay() override;

	virtual void EndPlay(EEndPlayReason::Type const EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	
	UFUNCTION(BlueprintNativeEvent)
	void OnNotifyGameReady();
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGameStartEvent);
	UPROPERTY(BlueprintAssignable)
	FGameStartEvent OnGameStart;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGameOverEvent);
	UPROPERTY(BlueprintAssignable)
	FGameOverEvent OnGameOver;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGameResumedEvent);
	UPROPERTY(BlueprintAssignable)
	FGameOverEvent OnGameResumed;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGamePausedEvent);
	UPROPERTY(BlueprintAssignable)
	FGameStartEvent OnGamePaused;

	UFUNCTION(BlueprintCallable)
	void ExecuteGameStart();
	
	UFUNCTION(BlueprintCallable)
	void ExecuteGameOver(AController* ResponsibleController, bool bShouldRestart);

	UFUNCTION(BlueprintCallable)
	void ResumeGame();
	
	UFUNCTION(BlueprintCallable)
	void PauseGame();

	UFUNCTION(BlueprintCallable)
	void ResetHighscore();

	UFUNCTION(BlueprintCallable)
	void ChangeWindowMode(EWindowMode::Type NewWindowMode);
	
	UFUNCTION(BlueprintPure)
	bool IsGameRunning() const;

	UFUNCTION(BlueprintPure)
	bool ShouldRestartGame() const;

	UFUNCTION(BlueprintPure)
	bool HasNewHighscore() const;
	
	UFUNCTION(BlueprintPure)
	int32 GetPlayerScore() const;
	
	UFUNCTION(BlueprintPure)
	int32 GetPlayerHighscore() const;

	UFUNCTION(BlueprintPure)
	AController* GetGameOverResponsibleController() const;

	UFUNCTION(BlueprintPure)
	UMXFPSSaveGameData* GetSaveGameData() const;

protected:
	UFUNCTION(BlueprintCallable)
	void RestartGame();
	
	// Whether this instance of the GameMode class loads/stores any SaveGame data from/to disk
	// This allows us to change any value in the SaveGame without necessarily using it.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MXFPS|Config")
	bool bModifySaveGame = false;

	// Whether this instance of the GameMode class should use data from the SaveGame to configure the game.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MXFPS|Config", meta = (EditCondition = "bModifySaveGame"))
	bool bUseSaveGame = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MXFPS|Player")
	bool bUseRandomPlayerSpawn = false;

	// The minimum distance that the enemies are spawned away from the player
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MXFPS|Player")
	float PlayerSafeRadius = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MXFPS|AI")
	TSubclassOf<APawn> EnemyClassPtr;
	
	// Number of stalker enemies that will be spawned
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MXFPS|AI", meta = (UIMin = 4, UIMax = 20))
	int32 NumEnemies = 4;

	// Speed of the stalker enemies in meters/sec
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MXFPS|AI", meta = (UIMin = 1, UIMax = 10))
	int32 EnemySpeed = 5;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MXFPS|Map")
	TSoftObjectPtr<class ARecastNavMesh> NavMeshPtr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MXFPS|Map")
	FVector LevelOrigin = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MXFPS|Map")
	float LevelRadius = 10000.f;

private:
	void EnableAllInputAndMovement();
	void DisableAllInputAndMovement();

	void LoadSaveGame();
	void StoreSaveGame();

	static void CacheBestResolutionPerWindowMode();

	UPROPERTY()
	UMXFPSSaveGameData* SaveGameData;

	UPROPERTY()
	AController* GameOverResponsibleController;
	
	int32 PlayerScore;
	int32 PlayerScoreOffset; // Used to keep track of PlayerScore between pauses

	FVector PlayerSpawnLocation;
	FDateTime GameRunningTime;
	bool bIsGameRunning;
	bool bIsGamePaused;
	bool bShouldRestartGame;

};
