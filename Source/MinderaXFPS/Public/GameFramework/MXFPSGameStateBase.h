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
	virtual void BeginPlay() override;

	virtual void EndPlay(EEndPlayReason::Type const EndPlayReason) override;

};
