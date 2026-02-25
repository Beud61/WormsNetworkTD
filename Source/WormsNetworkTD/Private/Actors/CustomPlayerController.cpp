// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/CustomPlayerController.h"
#include "WormsGameInstance.h"

void ACustomPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!MappingContextBase)
		return;

	if (GetLocalPlayer())
	{
		if (TObjectPtr<UEnhancedInputLocalPlayerSubsystem> InputSystem =
			GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSystem->AddMappingContext(MappingContextBase, 0);
		}
	}

	MyPlayer = Cast<ACustomPaperCharacter>(GetPawn());

	// On n'affiche le menu que si la partie n'a pas encore commencé.
	// Sans ce guard, BeginPlay recrée le menu sur la nouvelle map
	// après le ServerTravel car le PlayerController survit au travel.
	if (IsLocalController())
	{
		UWormsGameInstance* GI = Cast<UWormsGameInstance>(GetGameInstance());
		if (!GI || !GI->bGameStarted)
		{
			ShowMainMenu();
		}
	}
}

void ACustomPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACustomPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (TObjectPtr<UEnhancedInputComponent> EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(InputComponent))
	{
		for (FInputActionSetup i : IA_Setup)
		{
			EnhancedInputComponent->BindAction(i.Action, i.Event, this, i.ActionName.GetMemberName());
		}
	}
}

// ============================================================
//  Travel
// ============================================================

void ACustomPlayerController::ClientTravelInternal_Implementation(const FString& URL,
	ETravelType TravelType, bool bSeamless, const FGuid& MapPackageGuid)
{
	// Pose le flag dans le GameInstance (survit au travel) et cache le menu
	// AVANT que le Super déclenche le vrai travel et détruise le monde.
	UE_LOG(LogTemp, Warning, TEXT("ClientTravelInternal: URL=%s"), *URL);
	if (UWormsGameInstance* GI = Cast<UWormsGameInstance>(GetGameInstance()))
	{
		GI->bGameStarted = true;
	}
	HideMainMenu();

	Super::ClientTravelInternal_Implementation(URL, TravelType, bSeamless, MapPackageGuid);
}

// ============================================================
//  RPC Server
// ============================================================

void ACustomPlayerController::Server_StartGame_Implementation()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// S'exécute sur le serveur : on peut itérer tous les PC connectés
	// et leur envoyer le RPC Client pour cacher leur menu.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (ACustomPlayerController* PC = Cast<ACustomPlayerController>(It->Get()))
		{
			PC->Client_NotifyGameStarting();
		}
	}

	// TODO : remplacer par le chemin de ta vraie map de jeu.
	World->ServerTravel(TEXT("/Game/Maps/Lobby"));
}

// ============================================================
//  RPC Client
// ============================================================

void ACustomPlayerController::Client_NotifyGameStarting_Implementation()
{
	// Appelé par le serveur sur chaque PC connecté juste avant ServerTravel.
	UE_LOG(LogTemp, Warning, TEXT("Client_NotifyGameStarting appele."));
	if (UWormsGameInstance* GI = Cast<UWormsGameInstance>(GetGameInstance()))
	{
		GI->bGameStarted = true;
	}
	HideMainMenu();
}

// ============================================================
//  Input
// ============================================================

void ACustomPlayerController::Move(const FInputActionValue& Value)
{
	float Movement = Value.Get<float>();
	if (!MyPlayer) return;

	MyPlayer->AddMovementInput(FVector::ForwardVector, Movement);

	if (Movement != 0.f)
	{
		MyPlayer->Server_SetFacingDirection(Movement);
	}
}

void ACustomPlayerController::Jump(const FInputActionValue& Value)
{
	if (!MyPlayer) return;
	MyPlayer->Jump();
}

// ============================================================
//  UI
// ============================================================

void ACustomPlayerController::ShowMainMenu()
{
	if (!MenuWidgetClass) return;

	if (!MenuWidgetInstance)
	{
		MenuWidgetInstance = CreateWidget<UUIMenu>(this, MenuWidgetClass);
	}

	if (MenuWidgetInstance)
	{
		MenuWidgetInstance->AddToViewport();
	}
}

void ACustomPlayerController::HideMainMenu()
{
	if (MenuWidgetInstance)
	{
		MenuWidgetInstance->CloseMenu();
		// On garde la référence pour ne pas recréer le widget
		// si ShowMainMenu est rappelé (ex: retour au menu principal).
	}
}