#include "Actors/CustomPlayerController.h"
#include "WormsGameInstance.h"

// ============================================================
//  BeginPlay
// ============================================================

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

	// Affiche le curseur souris
	bShowMouseCursor = true;
	SetInputMode(FInputModeGameAndUI());

	if (IsLocalController())
	{
		UWormsGameInstance* GI = Cast<UWormsGameInstance>(GetGameInstance());
		if (!GI || !GI->bGameStarted)
		{
			//ShowMainMenu(); // Remove for build game
		}
	}
}

// ============================================================
//  Tick
// ============================================================

void ACustomPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// ============================================================
//  SetupInputComponent
// ============================================================

void ACustomPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	TObjectPtr<UEnhancedInputComponent> EIC =
		Cast<UEnhancedInputComponent>(InputComponent);

	if (!EIC) return;

	// Binds génériques configurés dans le tableau IA_Setup
	for (const FInputActionSetup& Setup : IA_Setup)
	{
		EIC->BindAction(Setup.Action, Setup.Event, this, Setup.ActionName.GetMemberName());
	}
}

// ============================================================
//  Travel
// ============================================================

void ACustomPlayerController::ClientTravelInternal_Implementation(const FString& URL,
	ETravelType TravelType, bool bSeamless, const FGuid& MapPackageGuid)
{
	UE_LOG(LogTemp, Warning, TEXT("ClientTravelInternal: URL=%s"), *URL);
	if (UWormsGameInstance* GI = Cast<UWormsGameInstance>(GetGameInstance()))
	{
		GI->bGameStarted = true;
	}
	HideMainMenu();

	Super::ClientTravelInternal_Implementation(URL, TravelType, bSeamless, MapPackageGuid);
}

// ============================================================
//  RPC Client — Game Starting
// ============================================================

void ACustomPlayerController::Client_NotifyGameStarting_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("Client_NotifyGameStarting appele."));
	if (UWormsGameInstance* GI = Cast<UWormsGameInstance>(GetGameInstance()))
	{
		GI->bGameStarted = true;
	}
	HideMainMenu();
}

// ============================================================
//  Input — Move / Jump
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
//  Input — Fire
// ============================================================

void ACustomPlayerController::Fire(const FInputActionValue& Value)
{
	if (!IsLocalController() || !MyPlayer) return;

	// Cooldown
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastFireTime < FireCooldown) return;
	LastFireTime = Now;

	FVector RayOrigin, RayDir;
	if (!DeprojectMousePositionToWorld(RayOrigin, RayDir))
	{
		//UE_LOG(LogTemp, Warning, TEXT("[PC] Fire : DeprojectMousePositionToWorld a échoué."));
		return;
	}

	const FVector CharLocation = MyPlayer->GetActorLocation();
	const float CharY = CharLocation.Y;

	FVector MouseWorldPos;
	if (FMath::Abs(RayDir.Y) > KINDA_SMALL_NUMBER)
	{
		float t = (CharY - RayOrigin.Y) / RayDir.Y;
		MouseWorldPos = RayOrigin + RayDir * t;
	}
	else
	{
		MouseWorldPos = RayOrigin;
	}


	const FVector2D CharPos2D(CharLocation.X, CharLocation.Z);
	const FVector2D MousePos2D(MouseWorldPos.X, MouseWorldPos.Z);

	FVector2D Dir2D = MousePos2D - CharPos2D;
	if (Dir2D.IsNearlyZero()) return;
	Dir2D.Normalize();

	const FVector Direction3D(Dir2D.X, 0.f, Dir2D.Y);
	const FVector ProjectileSpawnPos = CharLocation + Direction3D * SpawnOffset;

	SpawnProjectile(ProjectileSpawnPos, Direction3D);
}

// ============================================================
//  SpawnProjectile
// ============================================================

void ACustomPlayerController::SpawnProjectile(FVector ProjectileSpawnPos, FVector Direction)
{
	if (!BulletBombClass || !MyPlayer) return;

	FActorSpawnParameters Params;
	Params.Owner = MyPlayer;
	Params.Instigator = MyPlayer;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ABulletBomb* Projectile = GetWorld()->SpawnActor<ABulletBomb>(
		BulletBombClass, ProjectileSpawnPos, FRotator::ZeroRotator, Params);

	if (Projectile)
	{
		Projectile->Launch(Direction);
		UE_LOG(LogTemp, Log, TEXT("[PC] Projectile spawné -> pos=(%.1f, %.1f) dir=(%.2f, %.2f)"),
			ProjectileSpawnPos.X, ProjectileSpawnPos.Z, Direction.X, Direction.Z);
	}
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
	}
}