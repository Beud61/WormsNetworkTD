// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/BoxComponent.h"
#include "DestructibleMap.generated.h"

UCLASS()
class WORMSNETWORKTD_API ADestructibleMap : public AActor
{
    GENERATED_BODY()

public:
    ADestructibleMap();
    virtual void BeginPlay() override;

    // Declenche une explosion a une position World (X = horizontal, Y = vertical)
    UFUNCTION(BlueprintCallable, Category = "Destruction")
    void ApplyExplosion(FVector2D WorldPosition, float Radius);

    // Verifie si un point World est dans le terrain solide
    UFUNCTION(BlueprintCallable, Category = "Destruction")
    bool IsSolid(FVector2D WorldPosition) const;

    // Lit les pixels du RT cote CPU (a appeler apres une explosion)
    UFUNCTION(BlueprintCallable, Category = "Destruction")
    void RefreshPixelData();

protected:
    // Le PNG de la map
    UPROPERTY(EditAnywhere, Category = "Map")
    TObjectPtr<UTexture2D> MapTexture;

    // Le material applique au plane (doit avoir MapTexture + DestructionMask)
    UPROPERTY(EditAnywhere, Category = "Map")
    TObjectPtr<UMaterialInterface> MapMaterial;

    // Material pour peindre les trous (cercle noir, voir guide BP)
    UPROPERTY(EditAnywhere, Category = "Map")
    TObjectPtr<UMaterialInterface> EraseMaterial;

    // Taille de la map en units UE (cm). Adapte a ton plane existant.
    UPROPERTY(EditAnywhere, Category = "Map")
    FVector2D MapWorldSize = FVector2D(4096.f, 1024.f);

    // Combien de cellules de collision en X et Y
    UPROPERTY(EditAnywhere, Category = "Map|Collision")
    int32 CollisionCellsX = 64;

    UPROPERTY(EditAnywhere, Category = "Map|Collision")
    int32 CollisionCellsY = 16;

    // Le mesh plane qui affiche la map
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map")
    TObjectPtr<UStaticMeshComponent> MapMesh;

private:
    // Render Target = masque de destruction (blanc = solide, noir = trou)
    UPROPERTY()
    TObjectPtr<UTextureRenderTarget2D> DestructionMask;

    // Instance dynamique du material
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> MapMatInstance;

    // Resolution du Render Target (mis a jour depuis la texture)
    int32 RTWidth = 2048;
    int32 RTHeight = 512;

    // Cache CPU du masque pour IsSolid() (mis a jour par RefreshPixelData)
    TArray<FColor> MaskPixels;
    bool bPixelDataDirty = false;

    // Grille de BoxColliders representant le terrain
    TArray<TObjectPtr<UBoxComponent>> CollisionBoxes;

    // --- Fonctions internes ---
    void InitRenderTarget();
    void GenerateInitialCollision();
    void UpdateCollisionAfterExplosion(FVector2D WorldPosition, float Radius);
    void ConvertWorldToUV(FVector2D WorldPos, float& U, float& V) const;
};