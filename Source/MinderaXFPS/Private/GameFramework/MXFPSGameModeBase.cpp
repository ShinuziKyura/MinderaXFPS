// Ricardo Santos, 2023

#include "GameFramework/MXFPSGameModeBase.h"

#include "GameFramework/MXFPSGameStateBase.h"

AMXFPSGameModeBase::AMXFPSGameModeBase(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
	, bUseRandomPlayerSpawn(false)
{
	GameStateClass = AMXFPSGameStateBase::StaticClass();
}