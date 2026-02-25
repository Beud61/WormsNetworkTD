// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/RoomInfoTemplate.h"

void URoomInfoTemplate::NativeConstruct()
{
    Super::NativeConstruct();
    if (Btn_JoinLobby)
    {
        Btn_JoinLobby->OnClicked.AddDynamic(this, &URoomInfoTemplate::HandleJoinClicked);
    }
}

void URoomInfoTemplate::UpdateValues_Implementation()
{
}

void URoomInfoTemplate::HandleJoinClicked()
{
    OnJoinClicked.Broadcast(SessionIndex);
}