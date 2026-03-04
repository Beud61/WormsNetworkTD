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
    MapMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADestructibleMap::BeginPlay()
{
    Super::BeginPlay();
    InitRenderTarget();
    BuildSolidPixels();
    RebuildSurfaceBoxes();
    if (bShowDebugCollision) DrawDebugCollision();
}

// ============================================================
//  InitRenderTarget
// ============================================================
void ADestructibleMap::InitRenderTarget()
{
    if (!MapTexture) { UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: MapTexture manquante")); return; }

    RTWidth = MapTexture->GetSizeX();
    RTHeight = MapTexture->GetSizeY();

    DestructionMask = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), RTWidth, RTHeight, RTF_RGBA8);
    if (!DestructionMask) return;
    UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), DestructionMask, FLinearColor::White);

    if (MapMaterial)
    {
        MapMatInstance = UMaterialInstanceDynamic::Create(MapMaterial, this);
        MapMatInstance->SetTextureParameterValue(TEXT("MapTexture"), MapTexture);
        MapMatInstance->SetTextureParameterValue(TEXT("DestructionMask"), DestructionMask);
        MapMesh->SetMaterial(0, MapMatInstance);
    }

    MapMesh->SetRelativeScale3D(FVector(MapWorldSize.X / 100.f, MapWorldSize.Y / 100.f, 1.f));
    UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: Init OK %dx%d"), RTWidth, RTHeight);
}

// ============================================================
//  BuildSolidPixels
// ============================================================
void ADestructibleMap::BuildSolidPixels()
{
    if (!MapTexture) return;

#if WITH_EDITOR
    FTexture2DMipMap& Mip = MapTexture->GetPlatformData()->Mips[0];
    if (!Mip.BulkData.IsBulkDataLoaded()) Mip.BulkData.LoadBulkDataWithFileReader();

    void* Data = Mip.BulkData.Lock(LOCK_READ_ONLY);
    if (Data)
    {
        const uint8* Px = static_cast<const uint8*>(Data);
        SolidPixels.SetNum(RTWidth * RTHeight);
        for (int32 i = 0; i < RTWidth * RTHeight; i++)
            SolidPixels[i] = Px[i * 4 + 3] > 10;
        Mip.BulkData.Unlock();
        UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: pixels lus OK (%dx%d)"), RTWidth, RTHeight);
        return;
    }
    Mip.BulkData.Unlock();
    UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: fallback RT..."));
#endif

    UTextureRenderTarget2D* TempRT = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), RTWidth, RTHeight, RTF_RGBA8);
    UCanvas* C; FVector2D CS; FDrawToRenderTargetContext Ctx;
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), TempRT, C, CS, Ctx);
    if (C) C->K2_DrawTexture(MapTexture, FVector2D(0, 0), FVector2D(RTWidth, RTHeight), FVector2D(0, 0));
    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Ctx);
    FRenderTarget* RT = TempRT->GameThread_GetRenderTargetResource();
    if (RT)
    {
        TArray<FColor> Tmp; RT->ReadPixels(Tmp);
        SolidPixels.SetNum(Tmp.Num());
        for (int32 i = 0; i < Tmp.Num(); i++) SolidPixels[i] = Tmp[i].A > 10;
        UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: fallback OK"));
    }
    TempRT->ConditionalBeginDestroy();
}

// ============================================================
//  Coordonnees
// ============================================================
FVector2D ADestructibleMap::PixelToWorld(float PX, float PY) const
{
    const FVector Loc = GetActorLocation();
    return FVector2D(
        (Loc.X - MapWorldSize.X * 0.5f) + (PX / RTWidth) * MapWorldSize.X,
        (Loc.Z + MapWorldSize.Y * 0.5f) - (PY / RTHeight) * MapWorldSize.Y
    );
}

void ADestructibleMap::ConvertWorldToUV(FVector2D WorldPos, float& U, float& V) const
{
    const FVector Loc = GetActorLocation();
    U = FMath::Clamp((WorldPos.X - (Loc.X - MapWorldSize.X * 0.5f)) / MapWorldSize.X, 0.f, 1.f);
    V = FMath::Clamp(1.f - (WorldPos.Y - (Loc.Z - MapWorldSize.Y * 0.5f)) / MapWorldSize.Y, 0.f, 1.f);
}

void ADestructibleMap::ConvertWorldToPixel(FVector2D WorldPos, int32& OutPX, int32& OutPY) const
{
    float U, V; ConvertWorldToUV(WorldPos, U, V);
    OutPX = FMath::Clamp((int32)(U * RTWidth), 0, RTWidth - 1);
    OutPY = FMath::Clamp((int32)(V * RTHeight), 0, RTHeight - 1);
}

bool ADestructibleMap::IsSolid(FVector2D WorldPosition) const
{
    if (SolidPixels.IsEmpty()) return false;
    int32 PX, PY; ConvertWorldToPixel(WorldPosition, PX, PY);
    return SolidPixels[PY * RTWidth + PX];
}

// ============================================================
//  SpawnBoxesForColumn
// ============================================================
void ADestructibleMap::SpawnBoxesForColumn(int32 PX)
{
    if (SolidPixels.IsEmpty()) return;
    PX = FMath::Clamp(PX, 0, RTWidth - 1);

    const FVector Loc = GetActorLocation();
    const float   HalfY = BoxHalfDepthY;

    float WorldLeft = PixelToWorld((float)PX, 0.f).X;
    float WorldRight = PixelToWorld((float)(PX + ContourStep), 0.f).X;
    float CenterX = (WorldLeft + WorldRight) * 0.5f;
    float HalfX = FMath::Abs(WorldRight - WorldLeft) * 0.5f;
    if (HalfX < 0.1f) return;

    auto SpawnBox = [&](float WorldZ, float ExtX, float ExtZ, float RotRoll)
        {
            UBoxComponent* Box = NewObject<UBoxComponent>(this);
            Box->RegisterComponent();
            Box->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
            Box->SetWorldLocation(FVector(CenterX, Loc.Y, WorldZ));
            Box->SetWorldRotation(FRotator(0.f, 0.f, RotRoll));
            Box->SetBoxExtent(FVector(ExtX, HalfY, ExtZ));
            Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Box->SetCollisionObjectType(ECC_WorldStatic);
            Box->SetCollisionResponseToAllChannels(ECR_Block);
            Box->SetVisibility(false);
            SurfaceBoxes.Add(Box);
        };

    bool  bPrevSolid = false;
    int32 SolidStartPY = 0;

    for (int32 PY = 0; PY < RTHeight; PY++)
    {
        bool bSolid = SolidPixels[PY * RTWidth + PX];

        if (bSolid && !bPrevSolid)
        {
            SolidStartPY = PY;
            FVector2D SurfWorld = PixelToWorld((float)PX, (float)PY);
            SpawnBox(SurfWorld.Y, HalfX, BoxHalfHeight, 0.f);
        }
        else if (!bSolid && bPrevSolid)
        {
            int32 WallHeightPx = PY - SolidStartPY;
            float WallHalfZ = FMath::Max(
                (float)WallHeightPx / RTHeight * MapWorldSize.Y * 0.5f,
                BoxHalfHeight
            );
            FVector2D MidWorld = PixelToWorld((float)PX, (float)(SolidStartPY + WallHeightPx / 2));
            SpawnBox(MidWorld.Y, HalfX, WallHalfZ, 0.f);
        }

        bPrevSolid = bSolid;
    }
}

// ============================================================
//  SpawnBoxesForRow
// ============================================================
void ADestructibleMap::SpawnBoxesForRow(int32 PY)
{
    if (SolidPixels.IsEmpty()) return;
    PY = FMath::Clamp(PY, 0, RTHeight - 1);

    const FVector Loc = GetActorLocation();
    const float   HalfY = BoxHalfDepthY;

    float WorldTop = PixelToWorld(0.f, (float)PY).Y;
    float WorldBottom = PixelToWorld(0.f, (float)(PY + ContourStep)).Y;
    float CenterZ = (WorldTop + WorldBottom) * 0.5f;
    float HalfZ = FMath::Abs(WorldTop - WorldBottom) * 0.5f;
    if (HalfZ < 0.1f) return;

    bool bPrevSolid = false;
    for (int32 PX = 0; PX < RTWidth; PX++)
    {
        bool bSolid = SolidPixels[PY * RTWidth + PX];

        if (bSolid && !bPrevSolid)
        {
            FVector2D WallWorld = PixelToWorld((float)PX, (float)PY);
            UBoxComponent* Box = NewObject<UBoxComponent>(this);
            Box->RegisterComponent();
            Box->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
            Box->SetWorldLocation(FVector(WallWorld.X, Loc.Y, CenterZ));
            Box->SetWorldRotation(FRotator::ZeroRotator);
            Box->SetBoxExtent(FVector(BoxHalfHeight, HalfY, HalfZ));
            Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Box->SetCollisionObjectType(ECC_WorldStatic);
            Box->SetCollisionResponseToAllChannels(ECR_Block);
            Box->SetVisibility(false);
            SurfaceBoxes.Add(Box);
        }
        else if (!bSolid && bPrevSolid)
        {
            FVector2D WallWorld = PixelToWorld((float)(PX - 1), (float)PY);
            UBoxComponent* Box = NewObject<UBoxComponent>(this);
            Box->RegisterComponent();
            Box->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
            Box->SetWorldLocation(FVector(WallWorld.X, Loc.Y, CenterZ));
            Box->SetWorldRotation(FRotator::ZeroRotator);
            Box->SetBoxExtent(FVector(BoxHalfHeight, HalfY, HalfZ));
            Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Box->SetCollisionObjectType(ECC_WorldStatic);
            Box->SetCollisionResponseToAllChannels(ECR_Block);
            Box->SetVisibility(false);
            SurfaceBoxes.Add(Box);
        }

        bPrevSolid = bSolid;
    }
}

// ============================================================
//  RebuildSurfaceBoxes
// ============================================================
void ADestructibleMap::RebuildSurfaceBoxes()
{
    for (UBoxComponent* Box : SurfaceBoxes)
        if (Box) Box->DestroyComponent();
    SurfaceBoxes.Empty();

    if (SolidPixels.IsEmpty()) return;

    for (int32 PX = 0; PX < RTWidth - ContourStep; PX += ContourStep)
        SpawnBoxesForColumn(PX);

    for (int32 PY = 0; PY < RTHeight - ContourStep; PY += ContourStep)
        SpawnBoxesForRow(PY);

    UE_LOG(LogTemp, Warning, TEXT("DestructibleMap: %d boxes creees"), SurfaceBoxes.Num());
}

// ============================================================
//  RebuildZone
// ============================================================
void ADestructibleMap::RebuildZone(int32 PixelXMin, int32 PixelXMax)
{
    FVector2D ZoneWorldLeft = PixelToWorld((float)PixelXMin, 0.f);
    FVector2D ZoneWorldRight = PixelToWorld((float)PixelXMax, 0.f);

    for (int32 i = SurfaceBoxes.Num() - 1; i >= 0; i--)
    {
        UBoxComponent* Box = SurfaceBoxes[i];
        if (!Box) continue;
        float BX = Box->GetComponentLocation().X;
        if (BX >= ZoneWorldLeft.X - 50.f && BX <= ZoneWorldRight.X + 50.f)
        {
            Box->DestroyComponent();
            SurfaceBoxes.RemoveAt(i);
        }
    }

    int32 PXMin = (PixelXMin / ContourStep) * ContourStep;
    int32 PXMax = FMath::Min(((PixelXMax / ContourStep) + 1) * ContourStep, RTWidth - ContourStep);

    for (int32 PX = PXMin; PX < PXMax; PX += ContourStep)
        SpawnBoxesForColumn(PX);

    for (int32 PY = 0; PY < RTHeight - ContourStep; PY += ContourStep)
        SpawnBoxesForRow(PY);
}

// ============================================================
//  DrawDebugCollision
// ============================================================
void ADestructibleMap::DrawDebugCollision()
{
    const UWorld* World = GetWorld();

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

    float U, V;
    ConvertWorldToUV(WorldPosition, U, V);
    float RadiusPx = (Radius / MapWorldSize.X) * RTWidth;

    UCanvas* Canvas = nullptr; FVector2D CanvasSize; FDrawToRenderTargetContext Context;
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), DestructionMask, Canvas, CanvasSize, Context);
    if (Canvas)
    {
        UMaterialInstanceDynamic* EraseInst = UMaterialInstanceDynamic::Create(EraseMaterial, this);
        Canvas->K2_DrawMaterial(EraseInst,
            FVector2D(U * RTWidth - RadiusPx, V * RTHeight - RadiusPx),
            FVector2D(RadiusPx * 2.f, RadiusPx * 2.f),
            FVector2D(0.f, 0.f), FVector2D(1.f, 1.f));
    }
    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);

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

    RebuildZone((int32)(CX - RadiusPx * 1.5f), (int32)(CX + RadiusPx * 1.5f));

    if (bShowDebugCollision) DrawDebugCollision();
}