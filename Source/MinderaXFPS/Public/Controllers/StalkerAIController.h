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

	UFUNCTION(BlueprintImplementableEvent, Category = "Stalker AI")
	void OnCanInitiateMoveTo();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Stalker AI")
	void OnShouldInterruptMoveTo();
	
	UFUNCTION(BlueprintCallable, Category = "Stalker AI")
	void InterruptMoveTo();

	UFUNCTION(BlueprintPure, Category = "Stalker AI")
	bool IsMoveToInProgress() const;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stalker AI")
	class UQulockComponent* QulockComponent;

private:
	bool bCachedCanMove;
	
};
