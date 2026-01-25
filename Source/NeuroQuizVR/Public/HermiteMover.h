#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HermiteMover.generated.h"

UCLASS()
class NEUROQUIZ_VR_API AHermiteMover : public AActor
{
    GENERATED_BODY()

public:
    AHermiteMover();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // --- Configuration ---

    // Le point de départ (P0)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hermite Spline", meta = (MakeEditWidget = true))
    FVector StartPoint;

    // La tangente au départ (M0) - C'est la vitesse de départ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hermite Spline", meta = (MakeEditWidget = true))
    FVector StartTangent;

    // Le point d'arrivée (P1)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hermite Spline", meta = (MakeEditWidget = true))
    FVector EndPoint;

    // La tangente à l'arrivée (M1) - C'est la vitesse d'arrivée
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hermite Spline", meta = (MakeEditWidget = true))
    FVector EndTangent;

    // Durée du mouvement en secondes
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hermite Spline")
    float Duration = 3.0f;

    // L'objet à déplacer (si vide, l'acteur se déplace lui-même)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hermite Spline")
    AActor* TargetActor;

    // --- Contrôles ---

    UFUNCTION(BlueprintCallable, Category = "Hermite Spline")
    void StartMovement();

    UFUNCTION(BlueprintCallable, Category = "Hermite Spline")
    void ResetMovement();

private:
    float CurrentTime;
    bool bIsMoving;

    // La fonction mathématique pure d'Hermite
    FVector CalculateHermite(FVector P0, FVector M0, FVector P1, FVector M1, float T);
};