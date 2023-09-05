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
	ALostSoulPlayerCharacter();

	virtual void BeginPlay();

	// TODO remove these functions if not needed
	/** Returns OnlyOwnerMesh subobject **/
	USkeletalMeshComponent* GetOnlyOwnerMesh() const { return OnlyOwnerMesh; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

protected:
	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* OnlyOwnerMesh; // TODO rename this

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	UCameraComponent* FirstPersonCameraComponent;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input)
	class UInputMappingContext* DefaultMappingContext; // TODO maybe rename this to SimpleMovementMappingContext
	
	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input)
	class UInputAction* LookAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input)
	class UInputAction* MoveAction;

	/** Jump Input Action */
//	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input)
//	class UInputAction* JumpAction;
	
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

	/** Called for movement input */
	void Move(FInputActionValue const& Value);

	/** Called for looking input */
	void Look(FInputActionValue const& Value);


};

