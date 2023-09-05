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

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MXFPS Config")
	bool bUseRandomPlayerSpawn;

};
