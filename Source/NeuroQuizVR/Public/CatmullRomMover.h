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
    ACatmullRomMover();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull-Rom Spline", meta = (MakeEditWidget = true))
    TArray<FVector> Points;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull-Rom Spline|Debug")
    bool bDrawDebugCurve = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull-Rom Spline|Debug", meta = (ClampMin = "4", ClampMax = "200"))
    int DebugSegments = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull-Rom Spline|Debug")
    bool bDrawDebugInEditor = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull-Rom Spline|Debug", meta = (ClampMin = "0.01", ClampMax = "10.0"))
    float DebugLifetime = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull-Rom Spline")
    float Duration = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull-Rom Spline")
    AActor* TargetActor;

    UFUNCTION(BlueprintCallable, Category = "Catmull-Rom Spline")
    void StartMovement();

    UFUNCTION(BlueprintCallable, Category = "Catmull-Rom Spline")
    void ResetMovement();

    virtual void OnConstruction(const FTransform& Transform) override;

private:
    float CurrentTime;
    bool bIsMoving;

    FVector BaseLocation;
    bool bBaseCaptured = false;

    FVector CalculateCatmullRom(float alpha, TArray<FVector> points);

#if WITH_EDITORONLY_DATA
    UPROPERTY(Transient)
    ULineBatchComponent* EditorLineBatch = nullptr;
#endif
};
