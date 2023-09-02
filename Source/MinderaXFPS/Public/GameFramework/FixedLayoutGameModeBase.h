// Ricardo Santos, 2023

#pragma once

#include <CoreMinimal.h>
#include <GameFramework/GameModeBase.h>

#include "FixedLayoutGameModeBase.generated.h"

UCLASS(Blueprintable)
class MINDERAXFPS_API AFixedLayoutGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	explicit AFixedLayoutGameModeBase(FObjectInitializer const& ObjectInitializer);
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bUseRandomPlayerSpawn;

};
