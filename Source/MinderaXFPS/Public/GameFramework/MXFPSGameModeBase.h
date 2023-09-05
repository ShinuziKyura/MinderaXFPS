// Ricardo Santos, 2023

#pragma once

#include <CoreMinimal.h>
#include <GameFramework/GameModeBase.h>

#include "MXFPSGameModeBase.generated.h"

UCLASS(Blueprintable)
class MINDERAXFPS_API AMXFPSGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	explicit AMXFPSGameModeBase(FObjectInitializer const& ObjectInitializer);
	
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnNotifyGameReady();
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnNotifyGameStart();
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnNotifyGameOver();

	UFUNCTION(BlueprintCallable)
	void ExecuteGameStart();
	
	UFUNCTION(BlueprintCallable)
	void ExecuteGameOver(APawn* ResponsibleActor);

	UFUNCTION(BlueprintCallable)
	void RestartGame();
	
	UFUNCTION(BlueprintPure)
	bool IsGameRunning() const;
	
	UFUNCTION(BlueprintPure)
	int32 GetPlayerScore() const;

	UFUNCTION(BlueprintPure)
	APawn* GetGameOverResponsibleActor() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MXFPS|Player")
	bool bUseRandomPlayerSpawn = false;

	// The minimum distance that the enemies are spawned away from the player
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MXFPS|Player")
	float PlayerSafeRadius = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MXFPS|AI")
	TSubclassOf<APawn> EnemyClassPtr;
	
	// Number of stalker enemies that will be spawned
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MXFPS|AI", meta = (UIMin = 4, UIMax = 16))
	int32 NumEnemies = 4;

	// Speed of the stalker enemies in meters/sec
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MXFPS|AI", meta = (UIMin = 1, UIMax = 10))
	int32 EnemySpeed = 4;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MXFPS|Map")
	TSoftObjectPtr<class ARecastNavMesh> NavMeshPtr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MXFPS|Map")
	FVector LevelOrigin = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MXFPS|Map")
	float LevelRadius = 10000.f;

private:
	void EnableAllInputAndMovement();
	void DisableAllInputAndMovement();

	UPROPERTY()
	APawn* GameOverResponsibleActor;

	FVector PlayerSpawnLocation;
	int32 PlayerScore;

	FDateTime GameStartTime;
	bool bIsGameRunning;
	
};
