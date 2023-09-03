// Ricardo Santos, 2023

#include "Actors/MXFPSPlayerCharacter.h"

#include <EnhancedInputComponent.h>
#include <EnhancedInputSubsystems.h>
#include <Animation/AnimInstance.h>
#include <Camera/CameraComponent.h>
#include <Components/CapsuleComponent.h>

//////////////////////////////////////////////////////////////////////////
// AMXFPSPlayerCharacter

AMXFPSPlayerCharacter::AMXFPSPlayerCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(50.f, 100.0f);
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true; // TODO hmmmm

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	OnlyOwnerMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterOnlyOwnerMesh"));
	OnlyOwnerMesh->SetOnlyOwnerSee(true);
	OnlyOwnerMesh->SetupAttachment(FirstPersonCameraComponent);
	OnlyOwnerMesh->bCastDynamicShadow = false;
	OnlyOwnerMesh->CastShadow = false;
	//OnlyOwnerMesh->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
	OnlyOwnerMesh->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

}

void AMXFPSPlayerCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (auto PlayerController = Cast<APlayerController>(Controller))
	{
		ULocalPlayer* Player = PlayerController->GetLocalPlayer();
		if (auto Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Player))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

}

//////////////////////////////////////////////////////////////////////////// Input

void AMXFPSPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//Jumping
	//	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
	//	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMXFPSPlayerCharacter::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMXFPSPlayerCharacter::Look);
	}
}


void AMXFPSPlayerCharacter::Move(FInputActionValue const& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AMXFPSPlayerCharacter::Look(FInputActionValue const& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}
