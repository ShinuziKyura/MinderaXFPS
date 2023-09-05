// Ricardo Santos, 2023

#pragma once

#include <CoreMinimal.h>
#include <Components/ActorComponent.h>

#include "QulockComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MINDERAXFPS_API UQulockComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UQulockComponent(FObjectInitializer const& ObjectInitializer);

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void SetupPlayerToCheck(APlayerController* PlayerController);

	UFUNCTION(BlueprintPure)
	bool CanMoveThisFrame() const;

	UFUNCTION(BlueprintPure)
	bool IsActorWithinPlayerView(APlayerController* Player, AActor* Actor) const; 

	UFUNCTION(BlueprintPure)
	bool IsActorWithinPlayerFrustum(APlayerController* Player, AActor* Actor,
									TArray<FVector>& OutSupportVertexes) const;
	
	UFUNCTION(BlueprintPure)
	FMatrix GetPlayerViewProjMatrix(APlayerController* Player) const;

	UFUNCTION(BlueprintPure)
	FVector GetPlayerViewOrigin(APlayerController* Player) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "QulockMovement")
	TSet<TEnumAsByte<EAutoReceiveInput::Type>> PlayersToCheck;
	
private:
	class UPlayerViewDataCachingSubsystem* GetCachingSubsystem() const;
	
	UPROPERTY()
	TSet<APlayerController*> PlayerControllerSet;

	UPROPERTY()
	mutable UPlayerViewDataCachingSubsystem* CachingSubsystem = nullptr;
	
	mutable TFrameValue<bool> bCanOwnerMove;
	
};