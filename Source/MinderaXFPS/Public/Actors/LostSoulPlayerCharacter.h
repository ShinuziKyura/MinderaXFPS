// Ricardo Santos, 2023

#pragma once

#include <CoreMinimal.h>
#include <InputActionValue.h>
#include <GameFramework/Character.h>

#include "LostSoulPlayerCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UAnimMontage;
class USoundBase;

UCLASS(config=Game)
class ALostSoulPlayerCharacter : public ACharacter
{
	GENERATED_BODY()
	
public:
	explicit ALostSoulPlayerCharacter(FObjectInitializer const& ObjectInitializer);

	virtual void BeginPlay() override;
	
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

protected:
	/** Pawn mesh: 1st person view (arms; seen only by self) */
	//UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	//USkeletalMeshComponent* OnlyOwnerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	UCameraComponent* FirstPersonCameraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input)
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input)
	class UInputAction* MoveAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input)
	class UInputAction* LookAction;

private:
	void Move(FInputActionValue const& Value);

	void Look(FInputActionValue const& Value);

};

