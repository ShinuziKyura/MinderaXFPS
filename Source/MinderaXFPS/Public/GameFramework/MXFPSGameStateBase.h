// Ricardo Santos, 2023

#pragma once

#include <CoreMinimal.h>
#include <GameFramework/GameStateBase.h>

#include "MXFPSGameStateBase.generated.h"

UCLASS()
class MINDERAXFPS_API AMXFPSGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	FMatrix GetPlayerViewMatrix() const;

	UFUNCTION(BlueprintPure)
	FVector GetPlayerViewOrigin() const;

//	UFUNCTION(BlueprintPure, DeprecatedFunction)
//	void GetPlayerFrustumEdges(FVector& TopLeftPos, FVector& TopLeftDir, FVector& BotLeftPos, FVector& BotLeftDir,
//							   FVector& TopRightPos, FVector& TopRightDir, FVector& BotRightPos, FVector& BotRightDir) const;
	
	UFUNCTION(BlueprintPure)
	bool IsActorWithinPlayerFrustum(AActor* Actor, TArray<FVector>& OutSupportVertexes) const;
	
protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(EEndPlayReason::Type const EndPlayReason) override; 

private:
	void UpdateViewMatrices(UWorld* World, ELevelTick LevelTick, float DeltaSeconds);

	FSceneViewInitOptions CachedViewInitOptions;
	FViewMatrices CachedViewMatrices;
	
};
