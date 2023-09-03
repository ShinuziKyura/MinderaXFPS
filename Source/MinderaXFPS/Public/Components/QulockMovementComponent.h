// Ricardo Santos, 2023

#pragma once

#include <CoreMinimal.h>
#include <GameFramework/CharacterMovementComponent.h>

#include "QulockMovementComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MINDERAXFPS_API UQulockMovementComponent : public /*UCharacterMovementComponent*/ UActorComponent
{
	GENERATED_BODY()

public:	
	UQulockMovementComponent(FObjectInitializer const& ObjectInitializer);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "QulockMovement")
	bool bCanOwnerMove;
	
};
