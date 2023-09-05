// Ricardo Santos, 2023

#pragma once

#include <CoreMinimal.h>
#include <AIController.h>

#include "StalkerAIController.generated.h"

UCLASS()
class MINDERAXFPS_API AStalkerAIController : public AAIController
{
	GENERATED_BODY()

public:
	explicit AStalkerAIController(FObjectInitializer const& ObjectInitializer);
	
protected:
	UFUNCTION(BlueprintCallable, Category = "Enemy Stalker")
	void InterruptMoveTo();

	UFUNCTION(BlueprintPure, Category = "Enemy Stalker")
	bool IsMovingToTarget() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Stalker")
	class UQulockComponent* QulockComponent;
	
};
