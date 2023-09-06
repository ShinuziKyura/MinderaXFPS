// Ricardo Santos, 2023

#include "Controllers/StalkerAIController.h"

#include "Components/QulockComponent.h"

AStalkerAIController::AStalkerAIController(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
	, QulockComponent(ObjectInitializer.CreateDefaultSubobject<UQulockComponent>(this, TEXT("Qulock")))
	, bShouldInterruptMoveTo(true)
	, bCachedCanMove(false)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AStalkerAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	bool bCanMove = QulockComponent->CanMoveThisFrame();

	bool bCanMoveStateChanged = bCanMove != bCachedCanMove;
	bCachedCanMove = bCanMove;
	
	if (bShouldInterruptMoveTo && bCanMoveStateChanged && !bCanMove && IsMoveToInProgress())
	{
		InterruptMoveTo();
		
		OnMoveToInterrupted();
	}
}

void AStalkerAIController::InterruptMoveTo()
{
	AAIController::StopMovement();
//	AController::StopMovement();
}

bool AStalkerAIController::IsMoveToInProgress() const
{
	return GetPathFollowingComponent()->GetStatus() == EPathFollowingStatus::Moving; 
}

bool AStalkerAIController::CanExecuteMoveTo() const
{
	return bCachedCanMove;
}
