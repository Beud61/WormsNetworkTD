// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/UIMenu.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UUIMenu::NativeConstruct()
{
	Super::NativeConstruct();
	
	SetupMenu();
}

void UUIMenu::SetupMenu()
{
	// Bind Events
	if (Btn_CreateRoom)
	{
		Btn_CreateRoom->OnClicked.AddDynamic(this, &UUIMenu::OnCreateRoomClicked);
	}

	if (Btn_JoinRoom)
	{
		Btn_JoinRoom->OnClicked.AddDynamic(this, &UUIMenu::OnJoinRoomClicked);
	}

	if (Btn_FindRoom)
	{		
		Btn_FindRoom->OnClicked.AddDynamic(this, &UUIMenu::OnFindRoomClicked);
	}

	if (Btn_Settings)
	{
		Btn_Settings->OnClicked.AddDynamic(this, &UUIMenu::OnSettingsClicked);
	}

	if (Btn_Quit)
	{
		Btn_Quit->OnClicked.AddDynamic(this, &UUIMenu::OnQuitClicked);
	}

	// Afficher le curseur
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly());
	}
}

void UUIMenu::OnCreateRoomClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Create Room clicked"));
	
	//TODO Create Session
	
}

void UUIMenu::OnJoinRoomClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Join Room clicked"));
	
	//TODO
}

void UUIMenu::OnFindRoomClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Find Room clicked"));

	//TODO
}

void UUIMenu::OnSettingsClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Settings clicked"));
	
	// TODO: Settings
}

void UUIMenu::OnQuitClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Quit clicked!"));
	
	// Quit
	if (APlayerController* PC = GetOwningPlayer())
	{
		UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
	}
}

void UUIMenu::CloseMenu()
{
	RemoveFromParent();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
}