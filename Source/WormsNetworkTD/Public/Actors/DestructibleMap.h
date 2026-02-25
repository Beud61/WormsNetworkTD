#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ProceduralMeshComponent.h"
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

    // 1 point tous les N pixels. 8 = bon compromis, 16 = leger
    UPROPERTY(EditAnywhere, Category = "Map|Collision")
    int32 ContourStep = 8;

    // Epaisseur du mesh de collision en Y (doit etre > demi-capsule du perso)
    UPROPERTY(EditAnywhere, Category = "Map|Collision")
    float CollisionThickness = 200.f;

    // Debug visuel au BeginPlay
    UPROPERTY(EditAnywhere, Category = "Map|Debug")
    bool bShowDebugCollision = false;

    UPROPERTY(EditAnywhere, Category = "Map|Debug")
    float DebugDuration = 60.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UStaticMeshComponent> MapMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UProceduralMeshComponent> CollisionMesh;

private:
    UPROPERTY()
    TObjectPtr<UTextureRenderTarget2D> DestructionMask;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> MapMatInstance;

    int32 RTWidth = 2048;
    int32 RTHeight = 512;

    TArray<bool> SolidPixels;

    void InitRenderTarget();
    void BuildSolidPixels();
    void RebuildCollisionMesh();

    // Retourne la hauteur world Z de la surface a un X world donne
    float GetSurfaceWorldZ(float WorldX) const;

    int32 GetSurfacePixelY(int32 PixelX) const;
    void ConvertWorldToPixel(FVector2D WorldPos, int32& OutPX, int32& OutPY) const;
    FVector2D PixelToWorld(float PX, float PY) const;
    void ConvertWorldToUV(FVector2D WorldPos, float& U, float& V) const;
};