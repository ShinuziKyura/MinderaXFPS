// Ricardo Santos, 2023

#include "Actors/LostSoulPlayerCharacter.h"

#include <EnhancedInputComponent.h>
#include <EnhancedInputSubsystems.h>
#include <Animation/AnimInstance.h>
#include <Camera/CameraComponent.h>
#include <Components/CapsuleComponent.h>

#include "GameFramework/MXFPSGameModeBase.h"

ALostSoulPlayerCharacter::ALostSoulPlayerCharacter(FObjectInitializer const& ObjectInitializer)
	: Super{ ObjectInitializer }
	, FirstPersonCameraComponent{ ObjectInitializer.CreateDefaultSubobject<UCameraComponent>(this, TEXT("FirstPersonCamera")) }
{
	GetCapsuleComponent()->InitCapsuleSize(50.f, 100.0f);
		
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

//	OnlyOwnerMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterOnlyOwnerMesh"));
//	OnlyOwnerMesh->SetOnlyOwnerSee(true);
//	OnlyOwnerMesh->SetupAttachment(FirstPersonCameraComponent);
//	OnlyOwnerMesh->bCastDynamicShadow = false;
//	OnlyOwnerMesh->CastShadow = false;
//	//OnlyOwnerMesh->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
//	OnlyOwnerMesh->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

}

void ALostSoulPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (auto PlayerController = Cast<APlayerController>(Controller))
	{
		ULocalPlayer* Player = PlayerController->GetLocalPlayer();
		if (auto Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Player))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

}

void ALostSoulPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALostSoulPlayerCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALostSoulPlayerCharacter::Look);

		FEnhancedInputActionEventBinding& Action = EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Triggered, this,
		                                                                             &ALostSoulPlayerCharacter::Pause);
	}
}


void ALostSoulPlayerCharacter::Move(FInputActionValue const& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void ALostSoulPlayerCharacter::Look(FInputActionValue const& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void ALostSoulPlayerCharacter::Pause(FInputActionValue const&)
{
	auto GameMode = GetWorld()->GetAuthGameMode<AMXFPSGameModeBase>();

	if (GameMode->IsGameRunning())
	{
		GameMode->PauseGame();
	}
}
