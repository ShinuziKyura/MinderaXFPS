// Ricardo Santos, 2023

#include "GameFramework/FixedLayoutGameModeBase.h"

#include "GameFramework/MXFPSGameStateBase.h"

AFixedLayoutGameModeBase::AFixedLayoutGameModeBase(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
	, bUseRandomPlayerSpawn(false)
{
	GameStateClass = AMXFPSGameStateBase::StaticClass();
}