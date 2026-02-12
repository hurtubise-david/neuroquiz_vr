// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

/**
 * 
 */
UCLASS()
class NEUROQUIZVR_API CatmullRomMover : public AActor
{
    GENERATED_BODY()

public:
	CatmullRomMover();

protected:
    virtual void BeginPlay() override;

#if WITH_EDITOR
    virtual void OnConstruction(const FTransform& Transform) override;
#endif

public:
    virtual void Tick(float DeltaTime) override;

    // --- Configuration ---

    // La tangente à l'arrivée (M1) - C'est la vitesse d'arrivée
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CatmullRom Spline")
    TArray<FVector> bControlPoints;

    // La tangente à l'arrivée (M1) - C'est la vitesse d'arrivée
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CatmullRom Spline", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    TArray<FVector> bTension;

    // Mode additif : EndPoint est interprété comme un OFFSET (Delta) plutôt qu'un point world
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CatmullRom Spline")
    bool bAdditive = false;

    // --- Ease In / Ease Out (timing) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CatmullRom Spline|Ease")
    bool bUseEaseInOut = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CatmullRom Spline|Ease", meta = (ClampMin = "0.1", ClampMax = "8.0"))
    float EaseExponent = 2.0f;

    // --- Debugs lines ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CatmullRom Spline|Debug")
    bool bDrawDebugCurve = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CatmullRom Spline|Debug", meta = (ClampMin = "4", ClampMax = "200"))
    int DebugSegments = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CatmullRom Spline|Debug")
    bool bDrawDebugInEditor = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CatmullRom Spline|Debug", meta = (ClampMin = "0.01", ClampMax = "10.0"))
    float DebugLifetime = 0.25f;


    // Durée du mouvement en secondes
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CatmullRom Spline")
    float Duration = 3.0f;

    // L'objet à déplacer (si vide, l'acteur se déplace lui-même)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CatmullRom Spline")
    AActor* TargetActor;

    // --- Contrôles ---

    UFUNCTION(BlueprintCallable, Category = "CatmullRom Spline")
    void StartMovement();

    UFUNCTION(BlueprintCallable, Category = "CatmullRom Spline")
    void ResetMovement();

private:
    float CurrentTime;
    bool bIsMoving;

    // capturée au StartMovement
    FVector BaseLocation;
    bool bBaseCaptured = false;

    FVector CalculateTangent(int i);
    FVector CalculateSegmentWithHermit(int i, float t);

#if WITH_EDITORONLY_DATA
    UPROPERTY(Transient)
    ULineBatchComponent* EditorLineBatch = nullptr;
#endif

};
class ULineBatchComponent;