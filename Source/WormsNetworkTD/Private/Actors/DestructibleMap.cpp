// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/DestructibleMap.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ProceduralMeshComponent.h"

// ============================================================
//  Constructeur
// ============================================================
ADestructibleMap::ADestructibleMap()
{
    PrimaryActorTick.bCanEverTick = false;

    MapMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MapMesh"));
    RootComponent = MapMesh;
    MapMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(
        TEXT("/Engine/BasicShapes/Plane.Plane"));
    if (PlaneMesh.Succeeded())
        MapMesh->SetStaticMesh(PlaneMesh.Object);

    // Le mesh de collision est invisible - juste pour la physique
    CollisionMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CollisionMesh"));
    CollisionMesh->SetupAttachment(RootComponent);
    CollisionMesh->SetVisibility(false);
    CollisionMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionMesh->SetCollisionObjectType(ECC_WorldStatic);
    CollisionMesh->SetCollisionResponseToAllChannels(ECR_Block);
    CollisionMesh->bUseComplexAsSimpleCollision = true; // Utilise le mesh exact comme collider
}

// ============================================================
//  BeginPlay
// ============================================================
void ADestructibleMap::BeginPlay()
{
    Super::BeginPlay();
    InitRenderTarget();
    BuildPixelDataFromTexture();
    RebuildCollisionMesh();
}

// ============================================================
//  InitRenderTarget
// ============================================================
void ADestructibleMap::InitRenderTarget()
{
    if (!MapTexture)
    {
        UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: MapTexture non assignee !"));
        return;
    }

    RTWidth = MapTexture->GetSizeX();
    RTHeight = MapTexture->GetSizeY();

    DestructionMask = UKismetRenderingLibrary::CreateRenderTarget2D(
        GetWorld(), RTWidth, RTHeight, RTF_RGBA8);

    if (!DestructionMask) return;

    UKismetRenderingLibrary::ClearRenderTarget2D(
        GetWorld(), DestructionMask, FLinearColor::White);

    if (MapMaterial)
    {
        MapMatInstance = UMaterialInstanceDynamic::Create(MapMaterial, this);
        MapMatInstance->SetTextureParameterValue(TEXT("MapTexture"), MapTexture);
        MapMatInstance->SetTextureParameterValue(TEXT("DestructionMask"), DestructionMask);
        MapMesh->SetMaterial(0, MapMatInstance);
    }

    FVector Scale(MapWorldSize.X / 100.f, MapWorldSize.Y / 100.f, 1.f);
    MapMesh->SetRelativeScale3D(Scale);

    UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: Init OK  RT %dx%d"), RTWidth, RTHeight);
}

// ============================================================
//  BuildPixelDataFromTexture
//  Lit l'alpha du PNG pour savoir quels pixels sont solides
// ============================================================
void ADestructibleMap::BuildPixelDataFromTexture()
{
    if (!MapTexture) return;

    FTexture2DMipMap& Mip = MapTexture->GetPlatformData()->Mips[0];
    const int32 W = Mip.SizeX;
    const int32 H = Mip.SizeY;

    if (!Mip.BulkData.IsBulkDataLoaded())
        Mip.BulkData.LoadBulkDataWithFileReader();

    void* Data = Mip.BulkData.Lock(LOCK_READ_ONLY);

    if (Data)
    {
        const uint8* Pixels = static_cast<const uint8*>(Data);
        SolidPixels.SetNum(W * H);
        for (int32 i = 0; i < W * H; i++)
            SolidPixels[i] = Pixels[i * 4 + 3] > 10; // Alpha > 10 = solide
        Mip.BulkData.Unlock();
        UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: PNG lu OK (%dx%d)"), W, H);
    }
    else
    {
        Mip.BulkData.Unlock();

        // Fallback : dessine sur un RT temporaire et lit les pixels
        UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: Fallback RT..."));
        UTextureRenderTarget2D* TempRT = UKismetRenderingLibrary::CreateRenderTarget2D(
            GetWorld(), W, H, RTF_RGBA8);

        UCanvas* Canvas; FVector2D CanvasSize; FDrawToRenderTargetContext Ctx;
        UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), TempRT, Canvas, CanvasSize, Ctx);
        if (Canvas)
            Canvas->K2_DrawTexture(MapTexture, FVector2D(0, 0), FVector2D(W, H), FVector2D(0, 0));
        UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Ctx);

        FRenderTarget* RT = TempRT->GameThread_GetRenderTargetResource();
        if (RT)
        {
            TArray<FColor> TempPixels;
            RT->ReadPixels(TempPixels);
            SolidPixels.SetNum(W * H);
            for (int32 i = 0; i < TempPixels.Num(); i++)
                SolidPixels[i] = TempPixels[i].A > 10;
            UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: Fallback OK"));
        }
        TempRT->ConditionalBeginDestroy();
    }
}

// ============================================================
//  GetSurfaceY
//  Retourne le Y (pixel) de la surface du terrain a la colonne X
//  = le premier pixel solide en partant du haut
// ============================================================
int32 ADestructibleMap::GetSurfaceY(int32 PixelX) const
{
    if (SolidPixels.IsEmpty()) return RTHeight;
    PixelX = FMath::Clamp(PixelX, 0, RTWidth - 1);

    for (int32 PY = 0; PY < RTHeight; PY++)
    {
        if (SolidPixels[PY * RTWidth + PixelX])
            return PY;
    }
    return RTHeight; // Colonne vide (trou jusqu'en bas)
}

// ============================================================
//  UVToWorld — convertit UV [0,1] en position world
// ============================================================
FVector2D ADestructibleMap::UVToWorld(float U, float V) const
{
    const FVector ActorLoc = GetActorLocation();
    float WorldX = (ActorLoc.X - MapWorldSize.X * 0.5f) + U * MapWorldSize.X;
    float WorldZ = (ActorLoc.Z + MapWorldSize.Y * 0.5f) - V * MapWorldSize.Y;
    return FVector2D(WorldX, WorldZ);
}

// ============================================================
//  RebuildCollisionMesh
//  Genere UN seul mesh polygonal qui suit le contour du terrain
//  Beaucoup plus efficace que des milliers de boxes !
//
//  Structure du mesh :
//
//  Surface (points hauts) :  p0--p1--p2--p3 ...
//                             |   |   |   |
//  Fond (points bas fixes) : b0--b1--b2--b3 ...
//
//  On cree des quads entre chaque paire de colonnes
// ============================================================
void ADestructibleMap::RebuildCollisionMesh()
{
    if (SolidPixels.IsEmpty()) return;

    TArray<FVector>     Vertices;
    TArray<int32>       Triangles;
    TArray<FVector>     Normals;
    TArray<FVector2D>   UVs;
    TArray<FColor>      Colors;
    TArray<FProcMeshTangent> Tangents;

    const float Y_Depth = 50.f; // Epaisseur du mesh de collision (axe Y, invisible)
    const float BottomZ = GetActorLocation().Z - MapWorldSize.Y * 0.5f - 50.f; // Fond de la map

    // On parcourt les colonnes avec le pas de simplification
    // CollisionStep = 4 => 1 point tous les 4 pixels => -512 points pour 2048px
    int32 VertIndex = 0;

    for (int32 PX = 0; PX <= RTWidth - CollisionStep; PX += CollisionStep)
    {
        int32 PX2 = FMath::Min(PX + CollisionStep, RTWidth - 1);

        // Hauteur de surface pour cette colonne et la suivante
        int32 SurfY1 = GetSurfaceY(PX);
        int32 SurfY2 = GetSurfaceY(PX2);

        // Convertit en coordonnees world
        float U1 = (float)PX / RTWidth;
        float U2 = (float)PX2 / RTWidth;
        float V1 = (float)SurfY1 / RTHeight;
        float V2 = (float)SurfY2 / RTHeight;

        FVector2D World1 = UVToWorld(U1, V1);
        FVector2D World2 = UVToWorld(U2, V2);

        // 4 vertices du quad :
        // [0] haut-gauche   (surface colonne PX)
        // [1] haut-droite   (surface colonne PX2)
        // [2] bas-droite    (fond)
        // [3] bas-gauche    (fond)

        FVector v0(World1.X, -Y_Depth, World1.Y);  // haut-gauche
        FVector v1(World2.X, -Y_Depth, World2.Y);  // haut-droite
        FVector v2(World2.X, -Y_Depth, BottomZ);   // bas-droite
        FVector v3(World1.X, -Y_Depth, BottomZ);   // bas-gauche

        Vertices.Add(v0);
        Vertices.Add(v1);
        Vertices.Add(v2);
        Vertices.Add(v3);

        // Triangle 1 : 0,1,2
        Triangles.Add(VertIndex + 0);
        Triangles.Add(VertIndex + 1);
        Triangles.Add(VertIndex + 2);

        // Triangle 2 : 0,2,3
        Triangles.Add(VertIndex + 0);
        Triangles.Add(VertIndex + 2);
        Triangles.Add(VertIndex + 3);

        // Normales et UVs (pas importantes pour une collision invisible)
        FVector Normal(0.f, -1.f, 0.f);
        Normals.Add(Normal); Normals.Add(Normal);
        Normals.Add(Normal); Normals.Add(Normal);

        UVs.Add(FVector2D(U1, V1)); UVs.Add(FVector2D(U2, V2));
        UVs.Add(FVector2D(U2, 1.f)); UVs.Add(FVector2D(U1, 1.f));

        VertIndex += 4;
    }

    // Cree le mesh de collision
    CollisionMesh->CreateMeshSection(
        0,          // Section index
        Vertices,
        Triangles,
        Normals,
        UVs,
        Colors,
        Tangents,
        true        // true = genere la collision
    );

    UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: Collision mesh genere — %d vertices, %d triangles"),
        Vertices.Num(), Triangles.Num() / 3);
}

// ============================================================
//  ConvertWorldToUV
// ============================================================
void ADestructibleMap::ConvertWorldToUV(FVector2D WorldPos, float& U, float& V) const
{
    const FVector ActorLoc = GetActorLocation();
    U = (WorldPos.X - (ActorLoc.X - MapWorldSize.X * 0.5f)) / MapWorldSize.X;
    V = 1.f - (WorldPos.Y - (ActorLoc.Z - MapWorldSize.Y * 0.5f)) / MapWorldSize.Y;
    U = FMath::Clamp(U, 0.f, 1.f);
    V = FMath::Clamp(V, 0.f, 1.f);
}

// ============================================================
//  IsSolid
// ============================================================
bool ADestructibleMap::IsSolid(FVector2D WorldPosition) const
{
    if (SolidPixels.IsEmpty()) return false;

    float U, V;
    ConvertWorldToUV(WorldPosition, U, V);

    const int32 PX = FMath::Clamp((int32)(U * RTWidth), 0, RTWidth - 1);
    const int32 PY = FMath::Clamp((int32)(V * RTHeight), 0, RTHeight - 1);

    return SolidPixels[PY * RTWidth + PX];
}

// ============================================================
//  ApplyExplosion
// ============================================================
void ADestructibleMap::ApplyExplosion(FVector2D WorldPosition, float Radius)
{
    if (!DestructionMask || !EraseMaterial) return;

    float U, V;
    ConvertWorldToUV(WorldPosition, U, V);
    float RadiusPx = (Radius / MapWorldSize.X) * RTWidth;

    // 1. Peint le trou sur le Render Target (visuel)
    UCanvas* Canvas = nullptr;
    FVector2D CanvasSize;
    FDrawToRenderTargetContext Context;

    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
        GetWorld(), DestructionMask, Canvas, CanvasSize, Context);

    if (Canvas)
    {
        UMaterialInstanceDynamic* EraseInst =
            UMaterialInstanceDynamic::Create(EraseMaterial, this);

        float CenterX = U * RTWidth;
        float CenterY = V * RTHeight;
        float Diam = RadiusPx * 2.f;

        Canvas->K2_DrawMaterial(EraseInst,
            FVector2D(CenterX - RadiusPx, CenterY - RadiusPx),
            FVector2D(Diam, Diam),
            FVector2D(0.f, 0.f), FVector2D(1.f, 1.f));
    }

    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);

    // 2. Met a jour le cache CPU (SolidPixels)
    UpdatePixelsAfterExplosion(WorldPosition, Radius);

    // 3. Reconstruit le mesh de collision avec le nouveau terrain
    //    On ne reconstruit que la zone affectee pour les performances
    RebuildCollisionMeshZone(WorldPosition, Radius);
}

// ============================================================
//  UpdatePixelsAfterExplosion
//  Marque les pixels dans le rayon comme vides
// ============================================================
void ADestructibleMap::UpdatePixelsAfterExplosion(FVector2D WorldPosition, float Radius)
{
    float U, V;
    ConvertWorldToUV(WorldPosition, U, V);

    float CX = U * RTWidth;
    float CY = V * RTHeight;
    float R = (Radius / MapWorldSize.X) * RTWidth;
    float R2 = R * R;

    int32 MinX = FMath::Clamp((int32)(CX - R) - 1, 0, RTWidth - 1);
    int32 MaxX = FMath::Clamp((int32)(CX + R) + 1, 0, RTWidth - 1);
    int32 MinY = FMath::Clamp((int32)(CY - R) - 1, 0, RTHeight - 1);
    int32 MaxY = FMath::Clamp((int32)(CY + R) + 1, 0, RTHeight - 1);

    for (int32 PY = MinY; PY <= MaxY; PY++)
        for (int32 PX = MinX; PX <= MaxX; PX++)
            if ((PX - CX) * (PX - CX) + (PY - CY) * (PY - CY) <= R2)
                SolidPixels[PY * RTWidth + PX] = false;
}

// ============================================================
//  RebuildCollisionMeshZone
//  Reconstruit SEULEMENT la zone autour de l'explosion
//  Plus performant que tout reconstruire
// ============================================================
void ADestructibleMap::RebuildCollisionMeshZone(FVector2D WorldPosition, float Radius)
{
    // Pour simplifier on reconstruit tout le mesh
    // Pour une version optimisee on pourrait ne reconstruire qu'une section
    // Mais avec CollisionStep=4 et 2048px => -512 quads, c'est tres rapide
    CollisionMesh->ClearAllMeshSections();
    RebuildCollisionMesh();
}