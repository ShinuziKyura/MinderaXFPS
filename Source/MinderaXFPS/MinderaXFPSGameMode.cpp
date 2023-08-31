// Copyright Epic Games, Inc. All Rights Reserved.

#include "MinderaXFPSGameMode.h"
#include "MinderaXFPSCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMinderaXFPSGameMode::AMinderaXFPSGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;
}

//void AMinderaXFPSGameMode::StartPlay()
//{
//	Super::StartPlay();
//
//	if (PawnClass)
//	{
//		DefaultPawnClass = PawnClass;
//	}
//}
