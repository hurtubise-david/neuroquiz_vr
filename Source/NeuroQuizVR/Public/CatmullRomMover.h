// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CatmullRomMover.generated.h"

UCLASS()
class NEUROQUIZVR_API ACatmullRomMover : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's propertiesS
    ACatmullRomMover();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // --- Configuration ---

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull-Rom Spline", meta = (MakeEditWidget = true))
    TArray<FVector> Points;

    // Durée du mouvement en secondes
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull-Rom Spline")
    float Duration = 3.0f;

    // L'objet à déplacer (si vide, l'acteur se déplace lui-même)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull-Rom Spline")
    AActor* TargetActor;

    // --- Contrôles ---

    // TODO : DEFINE THESE TO FIX BUILD

    //UFUNCTION(BlueprintCallable, Category = "Catmull-Rom Spline")
    //void StartMovement();

    //UFUNCTION(BlueprintCallable, Category = "Catmull-Rom Spline")
    //void ResetMovement();

private:
    float CurrentTime;
    bool bIsMoving;

    // capturée au StartMovement
    FVector BaseLocation;
    bool bBaseCaptured = false;

    // La fonction mathématique pure d'Hermite
    FVector CalculateCatmullRom(float alpha, TArray<FVector> points);

#if WITH_EDITORONLY_DATA
    UPROPERTY(Transient)
    ULineBatchComponent* EditorLineBatch = nullptr;
#endif
};
