// Ricardo Santos, 2023

#pragma once

#include <CoreMinimal.h>
#include <GameFramework/GameStateBase.h>

#include "MXFPSGameStateBase.generated.h"

// We'll keep some of the logic to determine the player frustum in the game state, as it is a singleton object,
// and the game logic requires only a single player.
// If this were to change later (e.g. multiple players), we would likely need to refactor this.

UCLASS()
class MINDERAXFPS_API AMXFPSGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	virtual void EndPlay(EEndPlayReason::Type const EndPlayReason) override;
	
	UFUNCTION(BlueprintPure)
	FMatrix GetPlayerViewProjMatrix() const;

	UFUNCTION(BlueprintPure)
	FVector GetPlayerViewOrigin() const;

	UFUNCTION(BlueprintPure)
	bool IsActorWithinPlayerFrustum(AActor* Actor, TArray<FVector>& OutSupportVertexes) const;

private:
	void UpdateViewMatrices(UWorld* World, ELevelTick LevelTick, float DeltaSeconds);

	FSceneViewInitOptions			CachedViewInitOptions;
	FViewMatrices					CachedViewMatrices;
	mutable TFrameValue<FMatrix>	CachedInvViewRotMat;

	FDelegateHandle PreActorTickDelegateHandle;

};
