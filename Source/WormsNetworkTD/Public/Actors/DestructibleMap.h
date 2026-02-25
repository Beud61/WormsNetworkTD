#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ProceduralMeshComponent.h"
#include "DestructibleMap.generated.h"

UCLASS()
class WORMSNETWORKTD_API ADestructibleMap : public AActor
{
    GENERATED_BODY()

public:
    ADestructibleMap();
    virtual void BeginPlay() override;

    // Declenche une explosion
    UFUNCTION(BlueprintCallable, Category = "Destruction")
    void ApplyExplosion(FVector2D WorldPosition, float Radius);

    // Verifie si un point est dans le terrain solide
    UFUNCTION(BlueprintCallable, Category = "Destruction")
    bool IsSolid(FVector2D WorldPosition) const;

protected:
    UPROPERTY(EditAnywhere, Category = "Map")
    TObjectPtr<UTexture2D> MapTexture;

    UPROPERTY(EditAnywhere, Category = "Map")
    TObjectPtr<UMaterialInterface> MapMaterial;

    UPROPERTY(EditAnywhere, Category = "Map")
    TObjectPtr<UMaterialInterface> EraseMaterial;

    UPROPERTY(EditAnywhere, Category = "Map")
    FVector2D MapWorldSize = FVector2D(4096.f, 1024.f);

    // Simplifie le contour : 1 = chaque pixel, 4 = 1 point tous les 4 pixels
    // Plus c'est grand, moins de polygones mais moins precis
    UPROPERTY(EditAnywhere, Category = "Map|Collision")
    int32 CollisionStep = 4;

    // Le mesh visuel (plane)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map")
    TObjectPtr<UStaticMeshComponent> MapMesh;

    // Le mesh de collision procedural (invisible, juste pour la physique)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map")
    TObjectPtr<UProceduralMeshComponent> CollisionMesh;

private:
    UPROPERTY()
    TObjectPtr<UTextureRenderTarget2D> DestructionMask;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> MapMatInstance;

    int32 RTWidth = 2048;
    int32 RTHeight = 512;

    // Cache CPU : true = solide, false = vide
    TArray<bool> SolidPixels;

    void InitRenderTarget();
    void BuildPixelDataFromTexture();
    void RebuildCollisionMesh();
    void UpdatePixelsAfterExplosion(FVector2D WorldPosition, float Radius);
    void ConvertWorldToUV(FVector2D WorldPos, float& U, float& V) const;
    FVector2D UVToWorld(float U, float V) const;

    // Trouve la hauteur du terrain a une colonne X en pixels
    int32 GetSurfaceY(int32 PixelX) const;
    void RebuildCollisionMeshZone(FVector2D WorldPosition, float Radius);
};