// Ricardo Santos, 2023

#include "Subsystem/PlayerViewDataCachingSubsystem.h"

DECLARE_STATS_GROUP(TEXT("Player ViewData Caching Subsystem"), STATGROUP_ViewDataCaching, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Update Player ViewData"), STAT_UpdatePlayerViewData, STATGROUP_ViewDataCaching);

void UPlayerViewDataCachingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	LastUpdatedFrame = 0;
}

void UPlayerViewDataCachingSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldPreActorTick.Remove(PreActorTickDelegateHandle);
	PreActorTickDelegateHandle.Reset();

	PlayerViewDataMap.Reset();
	
	Super::Deinitialize();
}

void UPlayerViewDataCachingSubsystem::SetupPlayerViewDataUpdate(APlayerController* PlayerController)
{
	ULocalPlayer* Player = PlayerController->GetLocalPlayer();
	FPlayerViewData& ViewDataRef = PlayerViewDataMap.Add(Player);
	
	if (!PreActorTickDelegateHandle.IsValid())
	{
		auto UpdateMemFuncPtr = &UPlayerViewDataCachingSubsystem::HandleUpdatePlayerViewData;
		PreActorTickDelegateHandle = FWorldDelegates::OnWorldPreActorTick.AddUObject(this, UpdateMemFuncPtr);
	}

	// We already ticked this frame, so update this player right now
	if (LastUpdatedFrame == GFrameCounter)
	{
		UpdatePlayerViewData(Player, ViewDataRef);
	}
}

FPlayerViewData UPlayerViewDataCachingSubsystem::GetPlayerViewData(APlayerController* PlayerController) const
{
	ULocalPlayer* Player = PlayerController->GetLocalPlayer();
	return LastUpdatedFrame == GFrameCounter ? PlayerViewDataMap.FindRef(Player) : FPlayerViewData{};
}

bool UPlayerViewDataCachingSubsystem::HasPlayerViewData(APlayerController* PlayerController) const
{
	return GetPlayerViewData(PlayerController).bIsValid;
}

bool UPlayerViewDataCachingSubsystem::DoesSupportWorldType(EWorldType::Type const WorldType) const
{
	return WorldType == EWorldType::Game
		|| WorldType == EWorldType::PIE;
}

void UPlayerViewDataCachingSubsystem::HandleUpdatePlayerViewData(UWorld* World, ELevelTick, float)
{
	if (World == GetWorld())
	{
		for (auto& PlayerViewDataPair : PlayerViewDataMap)
		{
			ULocalPlayer* Player = PlayerViewDataPair.Key;
			FPlayerViewData& ViewDataRef = PlayerViewDataPair.Value;

			UpdatePlayerViewData(Player, ViewDataRef);
		}

		LastUpdatedFrame = GFrameCounter;
	}
}

void UPlayerViewDataCachingSubsystem::UpdatePlayerViewData(ULocalPlayer* Player, FPlayerViewData& ViewDataRef)
{
	SCOPE_CYCLE_COUNTER(STAT_UpdatePlayerViewData);

	FSceneViewInitOptions ViewInitOptions;
	Player->CalcSceneViewInitOptions(ViewInitOptions, Player->ViewportClient->Viewport, nullptr);

	// We probably don't need the view matrices,
	// we can compute what we need directly from the init options;
	// note that FViewMatrices::Init does additional work based on the RHI/driver's state,
	// if stuff is breaking, use the ViewMatrices instead.
	
//	FViewMatrices ViewMatrices	= ViewInitOptions;

//	ViewDataRef.ViewProjMatrix		= ViewMatrices.GetViewProjectionMatrix();
	ViewDataRef.ViewProjMatrix		= ViewInitOptions.ComputeViewProjectionMatrix();
	ViewDataRef.InvViewRotMatrix	= ViewInitOptions.ViewRotationMatrix.InverseFast();
//	ViewDataRef.ViewOrigin			= ViewMatrices.GetViewOrigin();
	ViewDataRef.ViewOrigin			= ViewInitOptions.ViewOrigin;
	ViewDataRef.ViewRectangle		= ViewInitOptions.GetConstrainedViewRect();
	ViewDataRef.bIsValid			= true;
}
