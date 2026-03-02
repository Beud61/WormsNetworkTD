#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "DestructibleMap.generated.h"

UCLASS()
class WORMSNETWORKTD_API ADestructibleMap : public AActor
{
    GENERATED_BODY()

public:
    ADestructibleMap();
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Destruction")
    void ApplyExplosion(FVector2D WorldPosition, float Radius);

    UFUNCTION(BlueprintCallable, Category = "Destruction")
    bool IsSolid(FVector2D WorldPosition) const;

    UFUNCTION(BlueprintCallable, Category = "Destruction|Debug")
    void DrawDebugCollision();

protected:
    UPROPERTY(EditAnywhere, Category = "Map")
    TObjectPtr<UTexture2D> MapTexture;

    UPROPERTY(EditAnywhere, Category = "Map")
    TObjectPtr<UMaterialInterface> MapMaterial;

    UPROPERTY(EditAnywhere, Category = "Map")
    TObjectPtr<UMaterialInterface> EraseMaterial;

    UPROPERTY(EditAnywhere, Category = "Map")
    FVector2D MapWorldSize = FVector2D(4096.f, 1024.f);

    // 1 box tous les N pixels en X.
    // 4 = tres precis, 8 = bon compromis, 16 = leger
    UPROPERTY(EditAnywhere, Category = "Map|Collision")
    int32 ContourStep = 8;

    // Epaisseur des boxes en Y.
    // Doit largement depasser la capsule du perso des deux cotes.
    // Si le perso spawne a Y=0 et le BP_Map est a Y=0, 500 suffit.
    UPROPERTY(EditAnywhere, Category = "Map|Collision")
    float BoxHalfDepthY = 500.f;

    // Demi-hauteur de chaque box (fine = collision precise sur la surface)
    UPROPERTY(EditAnywhere, Category = "Map|Collision")
    float BoxHalfHeight = 12.f;

    UPROPERTY(EditAnywhere, Category = "Map|Debug")
    bool bShowDebugCollision = false;

    UPROPERTY(EditAnywhere, Category = "Map|Debug")
    float DebugDuration = 60.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UStaticMeshComponent> MapMesh;

private:
    UPROPERTY()
    TObjectPtr<UTextureRenderTarget2D> DestructionMask;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> MapMatInstance;

    int32 RTWidth = 2048;
    int32 RTHeight = 512;

    TArray<bool> SolidPixels;

    // Tableau des boxes de surface (une par segment de terrain)
    UPROPERTY()
    TArray<TObjectPtr<UBoxComponent>> SurfaceBoxes;

    void InitRenderTarget();
    void BuildSolidPixels();
    void RebuildSurfaceBoxes();

    UBoxComponent* CreateSurfaceBox(FVector2D SurfA, FVector2D SurfB);
    int32     GetSurfacePixelY(int32 PixelX) const;
    FVector2D PixelToWorld(float PX, float PY) const;
    void      ConvertWorldToUV(FVector2D WorldPos, float& U, float& V) const;
    void      ConvertWorldToPixel(FVector2D WorldPos, int32& OutPX, int32& OutPY) const;
};