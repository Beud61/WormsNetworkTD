

#include "Actors/DestructibleMap.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"

ADestructibleMap::ADestructibleMap()
{
    PrimaryActorTick.bCanEverTick = false;

    MapMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MapMesh"));
    RootComponent = MapMesh;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));

    if (PlaneMesh.Succeeded())
        MapMesh->SetStaticMesh(PlaneMesh.Object);

    MapMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADestructibleMap::BeginPlay()
{
    Super::BeginPlay();
    InitRenderTarget();
    RefreshPixelData();
    GenerateInitialCollision();
}


void ADestructibleMap::InitRenderTarget()
{
    if (!MapTexture)
    {
        UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: MapTexture non assignee !"));
        return;
    }

    RTWidth = MapTexture->GetSizeX();
    RTHeight = MapTexture->GetSizeY();

    // Cree un Render Target de la meme resolution que la texture
    DestructionMask = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), RTWidth, RTHeight, RTF_RGBA8_SRGB);

    if (!DestructionMask)
    {
        UE_LOG(LogTemp, Error, TEXT("DestructibleMap: Echec creation RenderTarget !"));
        return;
    }

    // White = No Destruction
    UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), DestructionMask, FLinearColor::White);

    if (MapMaterial)
    {
        MapMatInstance = UMaterialInstanceDynamic::Create(MapMaterial, this);
        MapMatInstance->SetTextureParameterValue(TEXT("MapTexture"), MapTexture);
        MapMatInstance->SetTextureParameterValue(TEXT("DestructionMask"), DestructionMask);
        MapMesh->SetMaterial(0, MapMatInstance);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: MapMaterial non assigne !"));
    }

    FVector Scale(MapWorldSize.X / 100.f, MapWorldSize.Y / 100.f, 1.f);
    MapMesh->SetRelativeScale3D(Scale);

    MaskPixels.Init(FColor::White, RTWidth * RTHeight);

    UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: Init OK — RT %dx%d"), RTWidth, RTHeight);
}


void ADestructibleMap::ConvertWorldToUV(FVector2D WorldPos, float& U, float& V) const
{
    const FVector ActorLoc = GetActorLocation();

    // Origin Up Left
    U = (WorldPos.X - (ActorLoc.X - MapWorldSize.X * 0.5f)) / MapWorldSize.X;
    V = 1.f - (WorldPos.Y - (ActorLoc.Z - MapWorldSize.Y * 0.5f)) / MapWorldSize.Y;

    U = FMath::Clamp(U, 0.f, 1.f);
    V = FMath::Clamp(V, 0.f, 1.f);
}

// ============================================================
//  ApplyExplosion
//  Put a black circle on the texture On the Pos
// ============================================================
void ADestructibleMap::ApplyExplosion(FVector2D WorldPosition, float Radius)
{
    if (!DestructionMask)
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyExplosion: DestructionMask null !"));
        return;
    }
    if (!EraseMaterial)
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyExplosion: EraseMaterial non assigne !"));
        return;
    }

    float U, V;
    ConvertWorldToUV(WorldPosition, U, V);


    float RadiusPx = (Radius / MapWorldSize.X) * RTWidth;

    UCanvas* Canvas = nullptr;
    FVector2D CanvasSize;
    FDrawToRenderTargetContext Context;

    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
        GetWorld(), DestructionMask, Canvas, CanvasSize, Context);

    if (Canvas)
    {
        float CenterX = U * RTWidth;
        float CenterY = V * RTHeight;
        float Diam = RadiusPx * 2.f;


        UMaterialInstanceDynamic* EraseInst =
            UMaterialInstanceDynamic::Create(EraseMaterial, this);


        Canvas->K2_DrawMaterial(
            EraseInst,
            FVector2D(CenterX - RadiusPx, CenterY - RadiusPx), // Position haut-gauche
            FVector2D(Diam, Diam),                               // Taille
            FVector2D(0.f, 0.f),                                 // UV min
            FVector2D(1.f, 1.f)                                  // UV max
        );
    }

    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);

    bPixelDataDirty = true;

    UpdateCollisionAfterExplosion(WorldPosition, Radius);
}

bool ADestructibleMap::IsSolid(FVector2D WorldPosition) const
{
    if (MaskPixels.IsEmpty()) return false;

    float U, V;
    ConvertWorldToUV(WorldPosition, U, V);

    const int32 PX = FMath::Clamp((int32)(U * RTWidth), 0, RTWidth - 1);
    const int32 PY = FMath::Clamp((int32)(V * RTHeight), 0, RTHeight - 1);

    // Blanc (R > 128) = solide | Noir = trou
    return MaskPixels[PY * RTWidth + PX].R > 128;
}

void ADestructibleMap::RefreshPixelData()
{
    if (!DestructionMask) return;
    if (!bPixelDataDirty && !MaskPixels.IsEmpty()) return;

    FRenderTarget* RT = DestructionMask->GameThread_GetRenderTargetResource();
    if (RT)
    {
        RT->ReadPixels(MaskPixels);
        bPixelDataDirty = false;
        UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: Pixel data rafraichi."));
    }
}


void ADestructibleMap::GenerateInitialCollision()
{

    for (UBoxComponent* Box : CollisionBoxes)
    {
        if (Box) Box->DestroyComponent();
    }
    CollisionBoxes.Empty();

    const float CellW = MapWorldSize.X / CollisionCellsX;
    const float CellH = MapWorldSize.Y / CollisionCellsY;
    const FVector ActorLoc = GetActorLocation();


    const float OriginX = ActorLoc.X - MapWorldSize.X * 0.5f;
    const float OriginZ = ActorLoc.Z + MapWorldSize.Y * 0.5f;

    for (int32 cx = 0; cx < CollisionCellsX; cx++)
    {
        for (int32 cy = 0; cy < CollisionCellsY; cy++)
        {

            float WorldX = OriginX + cx * CellW + CellW * 0.5f;
            float WorldZ = OriginZ - cy * CellH - CellH * 0.5f;

            FVector2D CellWorld2D(WorldX, WorldZ);


            if (!IsSolid(CellWorld2D)) continue;

            UBoxComponent* Box = NewObject<UBoxComponent>(this);
            Box->RegisterComponent();
            Box->AttachToComponent(RootComponent,
                FAttachmentTransformRules::KeepWorldTransform);
            Box->SetWorldLocation(FVector(WorldX, ActorLoc.Y, WorldZ));
            Box->SetBoxExtent(FVector(CellW * 0.5f, 50.f, CellH * 0.5f));
            Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Box->SetCollisionObjectType(ECC_WorldStatic);
            Box->SetCollisionResponseToAllChannels(ECR_Block);

            CollisionBoxes.Add(Box);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: %d BoxColliders generes"), CollisionBoxes.Num());
}


void ADestructibleMap::UpdateCollisionAfterExplosion(FVector2D WorldPosition, float Radius)
{

    RefreshPixelData();


    for (int32 i = CollisionBoxes.Num() - 1; i >= 0; i--)
    {
        UBoxComponent* Box = CollisionBoxes[i];
        if (!Box) continue;

        const FVector BoxLoc = Box->GetComponentLocation();
        const FVector2D BoxPos2D(BoxLoc.X, BoxLoc.Z);
        const float Dist = FVector2D::Distance(BoxPos2D, WorldPosition);


        if (Dist < Radius * 1.3f)   
        {
            Box->DestroyComponent();
            CollisionBoxes.RemoveAt(i);
        }
    }
}