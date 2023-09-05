// Ricardo Santos, 2023

#include "Controllers/StalkerAIController.h"

#include "Components/QulockComponent.h"

AStalkerAIController::AStalkerAIController(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
	, QulockComponent(ObjectInitializer.CreateDefaultSubobject<UQulockComponent>(this, TEXT("Qulock")))
{
}

bool AStalkerAIController::IsMovingToTarget() const
{
	return GetCurrentMoveRequestID() != FAIRequestID::InvalidRequest;
}

void AStalkerAIController::InterruptMoveTo()
{
	AAIController::StopMovement();
//	AController::StopMovement();
}
