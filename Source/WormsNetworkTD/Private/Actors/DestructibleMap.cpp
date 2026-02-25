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

    CollisionMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CollisionMesh"));
    CollisionMesh->SetupAttachment(RootComponent);
    CollisionMesh->SetVisibility(false);

    // IMPORTANT : simple collision uniquement
    // Le mesh est un solide extrade en Y — la capsule du perso le touche
    CollisionMesh->bUseComplexAsSimpleCollision = false;
    CollisionMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionMesh->SetCollisionObjectType(ECC_WorldStatic);
    CollisionMesh->SetCollisionResponseToAllChannels(ECR_Block);
}

// ============================================================
//  BeginPlay
// ============================================================
void ADestructibleMap::BeginPlay()
{
    Super::BeginPlay();
    InitRenderTarget();
    BuildSolidPixels();
    RebuildCollisionMesh();

    if (bShowDebugCollision)
        DrawDebugCollision();
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

    UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), DestructionMask, FLinearColor::White);

    if (MapMaterial)
    {
        MapMatInstance = UMaterialInstanceDynamic::Create(MapMaterial, this);
        MapMatInstance->SetTextureParameterValue(TEXT("MapTexture"), MapTexture);
        MapMatInstance->SetTextureParameterValue(TEXT("DestructionMask"), DestructionMask);
        MapMesh->SetMaterial(0, MapMatInstance);
    }

    FVector Scale(MapWorldSize.X / 100.f, MapWorldSize.Y / 100.f, 1.f);
    MapMesh->SetRelativeScale3D(Scale);

    UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: Init OK %dx%d"), RTWidth, RTHeight);
}

// ============================================================
//  BuildSolidPixels
// ============================================================
void ADestructibleMap::BuildSolidPixels()
{
    if (!MapTexture) return;

    FTexture2DMipMap& Mip = MapTexture->GetPlatformData()->Mips[0];
    if (!Mip.BulkData.IsBulkDataLoaded())
        Mip.BulkData.LoadBulkDataWithFileReader();

    void* Data = Mip.BulkData.Lock(LOCK_READ_ONLY);

    if (Data)
    {
        const uint8* Px = static_cast<const uint8*>(Data);
        SolidPixels.SetNum(RTWidth * RTHeight);
        for (int32 i = 0; i < RTWidth * RTHeight; i++)
            SolidPixels[i] = Px[i * 4 + 3] > 10;
        Mip.BulkData.Unlock();
        UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: %d pixels lus (BulkData)"), RTWidth * RTHeight);
        return;
    }

    Mip.BulkData.Unlock();

    // Fallback : via Render Target temporaire
    UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: BulkData vide, fallback RT..."));
    UTextureRenderTarget2D* TempRT = UKismetRenderingLibrary::CreateRenderTarget2D(
        GetWorld(), RTWidth, RTHeight, RTF_RGBA8);

    UCanvas* C; FVector2D CS; FDrawToRenderTargetContext Ctx;
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), TempRT, C, CS, Ctx);
    if (C) C->K2_DrawTexture(MapTexture, FVector2D(0, 0), FVector2D(RTWidth, RTHeight), FVector2D(0, 0));
    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Ctx);

    FRenderTarget* RT = TempRT->GameThread_GetRenderTargetResource();
    if (RT)
    {
        TArray<FColor> Tmp;
        RT->ReadPixels(Tmp);
        SolidPixels.SetNum(Tmp.Num());
        for (int32 i = 0; i < Tmp.Num(); i++)
            SolidPixels[i] = Tmp[i].A > 10;
        UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: Fallback OK"));
    }
    TempRT->ConditionalBeginDestroy();
}

// ============================================================
//  GetSurfacePixelY
//  Premier pixel solide depuis le haut a la colonne PX
// ============================================================
int32 ADestructibleMap::GetSurfacePixelY(int32 PixelX) const
{
    PixelX = FMath::Clamp(PixelX, 0, RTWidth - 1);
    for (int32 PY = 0; PY < RTHeight; PY++)
        if (SolidPixels[PY * RTWidth + PixelX]) return PY;
    return RTHeight;
}

// ============================================================
//  PixelToWorld
// ============================================================
FVector2D ADestructibleMap::PixelToWorld(float PX, float PY) const
{
    const FVector Loc = GetActorLocation();
    float WX = (Loc.X - MapWorldSize.X * 0.5f) + (PX / RTWidth) * MapWorldSize.X;
    float WZ = (Loc.Z + MapWorldSize.Y * 0.5f) - (PY / RTHeight) * MapWorldSize.Y;
    return FVector2D(WX, WZ);
}

void ADestructibleMap::ConvertWorldToUV(FVector2D WorldPos, float& U, float& V) const
{
    const FVector Loc = GetActorLocation();
    U = (WorldPos.X - (Loc.X - MapWorldSize.X * 0.5f)) / MapWorldSize.X;
    V = 1.f - (WorldPos.Y - (Loc.Z - MapWorldSize.Y * 0.5f)) / MapWorldSize.Y;
    U = FMath::Clamp(U, 0.f, 1.f);
    V = FMath::Clamp(V, 0.f, 1.f);
}

void ADestructibleMap::ConvertWorldToPixel(FVector2D WorldPos, int32& OutPX, int32& OutPY) const
{
    float U, V;
    ConvertWorldToUV(WorldPos, U, V);
    OutPX = FMath::Clamp((int32)(U * RTWidth), 0, RTWidth - 1);
    OutPY = FMath::Clamp((int32)(V * RTHeight), 0, RTHeight - 1);
}

bool ADestructibleMap::IsSolid(FVector2D WorldPosition) const
{
    if (SolidPixels.IsEmpty()) return false;
    int32 PX, PY;
    ConvertWorldToPixel(WorldPosition, PX, PY);
    return SolidPixels[PY * RTWidth + PX];
}

float ADestructibleMap::GetSurfaceWorldZ(float WorldX) const
{
    const FVector Loc = GetActorLocation();
    float U = (WorldX - (Loc.X - MapWorldSize.X * 0.5f)) / MapWorldSize.X;
    U = FMath::Clamp(U, 0.f, 1.f);
    int32 PX = (int32)(U * RTWidth);
    int32 PY = GetSurfacePixelY(PX);
    FVector2D W = PixelToWorld((float)PX, (float)PY);
    return W.Y;
}

// ============================================================
//  RebuildCollisionMesh
//
//  Cree un mesh 3D extrade :
//  - Face avant  (Y = +CollisionThickness/2)
//  - Face arriere (Y = -CollisionThickness/2)
//  - La surface du terrain forme le bord superieur
//  - Le bas est fixe (fond de la map)
//
//  Structure d'un quad entre colonnes i et i+1 :
//
//  SurfI --- SurfI+1    <- haut (surface terrain)
//    |            |
//  BotI  --- BotI+1     <- bas (fond fixe)
//
//  On extrade ca en Y pour donner de l'epaisseur
//  => la capsule du Character peut collider dessus
// ============================================================
void ADestructibleMap::RebuildCollisionMesh()
{
    if (SolidPixels.IsEmpty()) return;

    TArray<FVector>          Verts;
    TArray<int32>            Tris;
    TArray<FVector>          Normals;
    TArray<FVector2D>        UVs;
    TArray<FColor>           Colors;
    TArray<FProcMeshTangent> Tangents;

    const float HalfThick = CollisionThickness * 0.5f;
    const FVector Loc = GetActorLocation();
    const float BottomZ = Loc.Z - MapWorldSize.Y * 0.5f - 20.f;

    // Collecte les points de surface (1 par ContourStep pixels)
    TArray<FVector2D> SurfacePoints;
    for (int32 PX = 0; PX < RTWidth; PX += ContourStep)
    {
        int32 SY = GetSurfacePixelY(PX);
        SurfacePoints.Add(PixelToWorld((float)PX, (float)SY));
    }
    // Dernier point (bord droit)
    SurfacePoints.Add(PixelToWorld((float)(RTWidth - 1),
        (float)GetSurfacePixelY(RTWidth - 1)));

    int32 N = SurfacePoints.Num();
    if (N < 2) return;

    // Pour chaque colonne, on cree 4 vertices (2 avant, 2 arriere)
    // puis on assemble les faces
    //
    // Index layout par colonne i :
    //   i*4+0 = Surface avant  (Y = +HalfThick)
    //   i*4+1 = Fond    avant  (Y = +HalfThick)
    //   i*4+2 = Surface arriere (Y = -HalfThick)
    //   i*4+3 = Fond    arriere (Y = -HalfThick)

    for (int32 i = 0; i < N; i++)
    {
        FVector2D Surf = SurfacePoints[i];
        Verts.Add(FVector(Surf.X, +HalfThick, Surf.Y));   // Surface avant
        Verts.Add(FVector(Surf.X, +HalfThick, BottomZ));  // Fond avant
        Verts.Add(FVector(Surf.X, -HalfThick, Surf.Y));   // Surface arriere
        Verts.Add(FVector(Surf.X, -HalfThick, BottomZ));  // Fond arriere

        Normals.Add(FVector(0, 0, 1)); Normals.Add(FVector(0, 0, -1));
        Normals.Add(FVector(0, 0, 1)); Normals.Add(FVector(0, 0, -1));
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(0, 1));
        UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1));
    }

    // Assemble les quads entre colonnes adjacentes
    for (int32 i = 0; i < N - 1; i++)
    {
        int32 A = i * 4;
        int32 B = (i + 1) * 4;

        // Face AVANT (Y = +HalfThick) : A0,B0,B1,A0,B1,A1
        Tris.Add(A + 0); Tris.Add(B + 0); Tris.Add(B + 1);
        Tris.Add(A + 0); Tris.Add(B + 1); Tris.Add(A + 1);

        // Face ARRIERE (Y = -HalfThick) : A2,B3,B2,A2,A3,B3
        Tris.Add(A + 2); Tris.Add(B + 3); Tris.Add(B + 2);
        Tris.Add(A + 2); Tris.Add(A + 3); Tris.Add(B + 3);

        // Face DESSUS (surface terrain) : A0,A2,B2,A0,B2,B0
        Tris.Add(A + 0); Tris.Add(A + 2); Tris.Add(B + 2);
        Tris.Add(A + 0); Tris.Add(B + 2); Tris.Add(B + 0);

        // Face DESSOUS (fond) : A1,B3,A3,A1,B1,B3
        Tris.Add(A + 1); Tris.Add(B + 3); Tris.Add(A + 3);
        Tris.Add(A + 1); Tris.Add(B + 1); Tris.Add(B + 3);
    }

    CollisionMesh->CreateMeshSection(0, Verts, Tris, Normals, UVs, Colors, Tangents,
        true); // true = genere la collision

    UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: CollisionMesh OK — %d colonnes, %d verts"),
        N, Verts.Num());
}

// ============================================================
//  DrawDebugCollision
//  Ligne verte sur la surface, jaune sur les bords
// ============================================================
void ADestructibleMap::DrawDebugCollision()
{
    if (SolidPixels.IsEmpty()) return;

    const UWorld* World = GetWorld();
    const float Y = GetActorLocation().Y;

    FVector2D Prev = PixelToWorld(0.f, (float)GetSurfacePixelY(0));

    for (int32 PX = ContourStep; PX < RTWidth; PX += ContourStep)
    {
        FVector2D Curr = PixelToWorld((float)PX, (float)GetSurfacePixelY(PX));

        DrawDebugLine(World,
            FVector(Prev.X, Y, Prev.Y),
            FVector(Curr.X, Y, Curr.Y),
            FColor::Green, false, DebugDuration, 0, 4.f);

        DrawDebugPoint(World,
            FVector(Curr.X, Y, Curr.Y),
            6.f, FColor::Red, false, DebugDuration);

        Prev = Curr;
    }

    // Bord gauche
    const FVector Loc = GetActorLocation();
    float BottomZ = Loc.Z - MapWorldSize.Y * 0.5f;
    FVector2D TopLeft = PixelToWorld(0.f, (float)GetSurfacePixelY(0));
    FVector2D TopRight = PixelToWorld((float)(RTWidth - 1), (float)GetSurfacePixelY(RTWidth - 1));
    DrawDebugLine(World, FVector(TopLeft.X, Y, TopLeft.Y), FVector(TopLeft.X, Y, BottomZ), FColor::Yellow, false, DebugDuration, 0, 3.f);
    DrawDebugLine(World, FVector(TopRight.X, Y, TopRight.Y), FVector(TopRight.X, Y, BottomZ), FColor::Yellow, false, DebugDuration, 0, 3.f);
    DrawDebugLine(World, FVector(TopLeft.X, Y, BottomZ), FVector(TopRight.X, Y, BottomZ), FColor::Yellow, false, DebugDuration, 0, 3.f);

    UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: Debug dessine"));
}

// ============================================================
//  ApplyExplosion
// ============================================================
void ADestructibleMap::ApplyExplosion(FVector2D WorldPosition, float Radius)
{
    if (!DestructionMask || !EraseMaterial) return;

    // 1. VISUEL : peint un cercle noir sur le Render Target
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
        UMaterialInstanceDynamic* EraseInst =
            UMaterialInstanceDynamic::Create(EraseMaterial, this);
        Canvas->K2_DrawMaterial(EraseInst,
            FVector2D(U * RTWidth - RadiusPx, V * RTHeight - RadiusPx),
            FVector2D(RadiusPx * 2.f, RadiusPx * 2.f),
            FVector2D(0.f, 0.f), FVector2D(1.f, 1.f));
    }

    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);

    // 2. MET A JOUR le cache CPU (SolidPixels)
    float CX = U * RTWidth;
    float CY = V * RTHeight;
    float R2px = (Radius / MapWorldSize.X) * RTWidth;
    R2px = R2px * R2px;

    int32 MinX = FMath::Clamp((int32)(CX - RadiusPx) - 1, 0, RTWidth - 1);
    int32 MaxX = FMath::Clamp((int32)(CX + RadiusPx) + 1, 0, RTWidth - 1);
    int32 MinY = FMath::Clamp((int32)(CY - RadiusPx) - 1, 0, RTHeight - 1);
    int32 MaxY = FMath::Clamp((int32)(CY + RadiusPx) + 1, 0, RTHeight - 1);

    for (int32 PY = MinY; PY <= MaxY; PY++)
        for (int32 PX2 = MinX; PX2 <= MaxX; PX2++)
            if ((PX2 - CX) * (PX2 - CX) + (PY - CY) * (PY - CY) <= R2px)
                SolidPixels[PY * RTWidth + PX2] = false;

    // 3. RECONSTRUIT la collision (tout le mesh, rapide avec ContourStep >= 8)
    CollisionMesh->ClearAllMeshSections();
    RebuildCollisionMesh();

    // 4. Redessine le debug si actif
    if (bShowDebugCollision)
        DrawDebugCollision();
}