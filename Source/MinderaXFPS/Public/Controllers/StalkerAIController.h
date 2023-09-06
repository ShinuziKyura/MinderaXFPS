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

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Stalker")
	void OnMoveToInterrupted();
	
	UFUNCTION(BlueprintCallable, Category = "Stalker")
	void InterruptMoveTo();

	UFUNCTION(BlueprintPure, Category = "Stalker")
	bool IsMoveToInProgress() const;

	UFUNCTION(BlueprintPure, Category = "Stalker")
	bool CanExecuteMoveTo() const;
	
protected:
	UPROPERTY(EditAnywhere, Category = "Stalker")
	class UQulockComponent* QulockComponent;
	
	// Whether we should interrupt any MoveTo call in progress
	// if the Qulock component determines that the actor should not move
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stalker")
	bool bShouldInterruptMoveTo;

private:
	bool bCachedCanMove;
	
};
