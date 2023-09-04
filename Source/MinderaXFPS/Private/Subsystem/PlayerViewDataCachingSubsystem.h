// Ricardo Santos, 2023

#pragma once

#include <CoreMinimal.h>
#include <Subsystems/WorldSubsystem.h>

#include "PlayerViewDataCachingSubsystem.generated.h"

USTRUCT()
struct FPlayerViewData
{
	GENERATED_BODY()
	
	FMatrix		ViewProjMatrix;
	FMatrix		InvViewRotMatrix;
	FVector		ViewOrigin;
	FIntRect	ViewRectangle;
	bool		bIsValid = false;
};

UCLASS()
class UPlayerViewDataCachingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void SetupPlayerViewDataUpdate(APlayerController* PlayerController);

	FPlayerViewData GetPlayerViewData(APlayerController* PlayerController) const;

	bool HasPlayerViewData(APlayerController* PlayerController) const;
	
protected:
	virtual bool DoesSupportWorldType(EWorldType::Type const WorldType) const override;

private:
	void HandleUpdatePlayerViewData(UWorld* World, ELevelTick LevelTickMode, float DeltaSeconds);
	void UpdatePlayerViewData(ULocalPlayer* Player, FPlayerViewData& ViewDataRef);

	UPROPERTY()
	TMap<ULocalPlayer*, FPlayerViewData> PlayerViewDataMap;
	uint64 LastUpdatedFrame;
	
	FDelegateHandle PreActorTickDelegateHandle;
	
};
