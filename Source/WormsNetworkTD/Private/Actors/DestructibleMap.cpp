#include "Actors/DestructibleMap.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"

// ============================================================
//  Constructeur
// ============================================================
ADestructibleMap::ADestructibleMap()
{
    PrimaryActorTick.bCanEverTick = false;

    MapMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MapMesh"));
    RootComponent = MapMesh;
    MapMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// ============================================================
//  BeginPlay
// ============================================================
void ADestructibleMap::BeginPlay()
{
    Super::BeginPlay();
    InitRenderTarget();
    BuildSolidPixels();
    RebuildSurfaceBoxes();

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

    // Fallback via Render Target temporaire
    UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: fallback RT..."));
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
        UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: fallback OK"));
    }
    TempRT->ConditionalBeginDestroy();
}

// ============================================================
//  GetSurfacePixelY
//  Premier pixel solide depuis le haut dans la colonne PX
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

// ============================================================
//  CreateSurfaceBox
//  Cree une BoxComponent fine et inclinee entre deux points
//  de surface — identique aux boxes jaunes de la demo JS.
//
//  Vue de cote :
//
//  A (surf) ---- B (surf)
//       \       /
//        [  box  ]   <- inclinee selon la pente A->B
//
//  Vue de face (axe X) :
//        |<-- BoxHalfDepthY * 2 -->|
//        |_________________________|
//        Y = ActorY - Depth ... Y = ActorY + Depth
// ============================================================
UBoxComponent* ADestructibleMap::CreateSurfaceBox(FVector2D SurfA, FVector2D SurfB)
{
    const FVector Loc = GetActorLocation();

    // Centre de la box entre les deux points de surface
    float CenterX = (SurfA.X + SurfB.X) * 0.5f;
    float CenterZ = (SurfA.Y + SurfB.Y) * 0.5f;
    float HalfX = FMath::Abs(SurfB.X - SurfA.X) * 0.5f;

    if (HalfX < 0.1f) return nullptr;

    // Calcul de l'angle de la pente (en degres dans le plan XZ)
    float DeltaX = SurfB.X - SurfA.X;
    float DeltaZ = SurfB.Y - SurfA.Y;
    float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(DeltaZ, DeltaX));

    UBoxComponent* Box = NewObject<UBoxComponent>(this);
    Box->RegisterComponent();
    Box->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);

    // Position : centree en Y sur l'acteur (meme Y que le perso doit spawner)
    Box->SetWorldLocation(FVector(CenterX, Loc.Y, CenterZ));

    // Rotation : inclinee selon la pente du terrain (Roll dans le plan XZ)
    // On utilise Roll car le mesh 2D est dans le plan XZ
    Box->SetWorldRotation(FRotator(0.f, 0.f, -AngleDeg));

    // Extent : large en X (couvre le segment), profond en Y (couvre la capsule), fin en Z
    Box->SetBoxExtent(FVector(HalfX, BoxHalfDepthY, BoxHalfHeight));

    Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Box->SetCollisionObjectType(ECC_WorldStatic);
    Box->SetCollisionResponseToAllChannels(ECR_Block);
    Box->SetVisibility(false);

    return Box;
}

// ============================================================
//  RebuildSurfaceBoxes
//  Detruit toutes les boxes et les recrée depuis SolidPixels.
//  Appele au BeginPlay et apres chaque explosion complete.
// ============================================================
void ADestructibleMap::RebuildSurfaceBoxes()
{
    // Detruit les anciennes boxes
    for (UBoxComponent* Box : SurfaceBoxes)
        if (Box) Box->DestroyComponent();
    SurfaceBoxes.Empty();

    if (SolidPixels.IsEmpty()) return;

    for (int32 PX = 0; PX < RTWidth - ContourStep; PX += ContourStep)
    {
        int32 SYA = GetSurfacePixelY(PX);
        int32 SYB = GetSurfacePixelY(PX + ContourStep);

        // Colonne completement vide : pas de box
        if (SYA >= RTHeight && SYB >= RTHeight) continue;

        FVector2D SurfA = PixelToWorld((float)PX, (float)SYA);
        FVector2D SurfB = PixelToWorld((float)(PX + ContourStep), (float)SYB);

        UBoxComponent* Box = CreateSurfaceBox(SurfA, SurfB);
        if (Box) SurfaceBoxes.Add(Box);
    }

    UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: %d boxes de surface creees"), SurfaceBoxes.Num());
}

// ============================================================
//  DrawDebugCollision
//  Ligne verte = contour de surface
//  Boites jaunes = boxes de collision avec leur inclinaison
// ============================================================
void ADestructibleMap::DrawDebugCollision()
{
    if (SolidPixels.IsEmpty()) return;

    const UWorld* World = GetWorld();
    const float   Y = GetActorLocation().Y;

    // Ligne de surface verte + points rouges
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
            8.f, FColor::Red, false, DebugDuration);
        Prev = Curr;
    }

    // Boites jaunes inclinées (la collision reelle)
    for (UBoxComponent* Box : SurfaceBoxes)
    {
        if (!Box) continue;
        DrawDebugBox(World,
            Box->GetComponentLocation(),
            FVector(Box->GetScaledBoxExtent().X, 4.f, Box->GetScaledBoxExtent().Z),
            Box->GetComponentQuat(),
            FColor::Yellow, false, DebugDuration, 0, 2.f);
    }

    UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: debug OK (%d boxes)"), SurfaceBoxes.Num());
}

// ============================================================
//  ApplyExplosion
// ============================================================
void ADestructibleMap::ApplyExplosion(FVector2D WorldPosition, float Radius)
{
    if (!DestructionMask || !EraseMaterial) return;

    // 1. VISUEL : cercle noir sur le Render Target
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

    // 2. MET A JOUR le cache CPU
    float CX = U * RTWidth;
    float CY = V * RTHeight;
    float R2px = RadiusPx * RadiusPx;

    int32 MinX = FMath::Clamp((int32)(CX - RadiusPx) - 1, 0, RTWidth - 1);
    int32 MaxX = FMath::Clamp((int32)(CX + RadiusPx) + 1, 0, RTWidth - 1);
    int32 MinY = FMath::Clamp((int32)(CY - RadiusPx) - 1, 0, RTHeight - 1);
    int32 MaxY = FMath::Clamp((int32)(CY + RadiusPx) + 1, 0, RTHeight - 1);

    for (int32 PY = MinY; PY <= MaxY; PY++)
        for (int32 PX = MinX; PX <= MaxX; PX++)
            if ((PX - CX) * (PX - CX) + (PY - CY) * (PY - CY) <= R2px)
                SolidPixels[PY * RTWidth + PX] = false;

    // 3. RECONSTRUCTION PARTIELLE des boxes dans la zone de l'explosion
    //    Supprime les boxes touchees
    for (int32 i = SurfaceBoxes.Num() - 1; i >= 0; i--)
    {
        UBoxComponent* Box = SurfaceBoxes[i];
        if (!Box) continue;
        FVector2D BoxPos(Box->GetComponentLocation().X, Box->GetComponentLocation().Z);
        if (FVector2D::Distance(BoxPos, WorldPosition) < Radius * 1.5f)
        {
            Box->DestroyComponent();
            SurfaceBoxes.RemoveAt(i);
        }
    }

    //    Recrée les boxes uniquement dans la zone affectee
    int32 PXMin = FMath::Clamp((int32)((CX - RadiusPx * 1.5f) / RTWidth * RTWidth), 0, RTWidth - 1);
    int32 PXMax = FMath::Clamp((int32)((CX + RadiusPx * 1.5f) / RTWidth * RTWidth), 0, RTWidth - 1);

    // Aligne sur ContourStep
    PXMin = (PXMin / ContourStep) * ContourStep;
    PXMax = FMath::Min(((PXMax / ContourStep) + 1) * ContourStep, RTWidth - ContourStep);

    for (int32 PX = PXMin; PX < PXMax; PX += ContourStep)
    {
        int32 SYA = GetSurfacePixelY(PX);
        int32 SYB = GetSurfacePixelY(PX + ContourStep);
        if (SYA >= RTHeight && SYB >= RTHeight) continue;

        FVector2D SurfA = PixelToWorld((float)PX, (float)SYA);
        FVector2D SurfB = PixelToWorld((float)(PX + ContourStep), (float)SYB);

        UBoxComponent* Box = CreateSurfaceBox(SurfA, SurfB);
        if (Box) SurfaceBoxes.Add(Box);
    }

    // 4. Debug
    if (bShowDebugCollision)
        DrawDebugCollision();
}