#include "HermiteMover.h"

AHermiteMover::AHermiteMover()
{
    PrimaryActorTick.bCanEverTick = true;
    bIsMoving = false;
    CurrentTime = 0.0f;

    // Valeurs par défaut pour la démo
    StartTangent = FVector(0, 0, 500); // Monte vers le haut au début
    EndTangent = FVector(0, 0, -500);  // Descend à la fin
}

void AHermiteMover::BeginPlay()
{
    Super::BeginPlay();

    // Si aucun point n'est défini, on prend la position actuelle comme départ
    if (StartPoint.IsZero())
    {
        StartPoint = GetActorLocation();
    }
}

void AHermiteMover::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsMoving)
    {
        CurrentTime += DeltaTime;

        // Normaliser le temps entre 0.0 et 1.0
        float t = FMath::Clamp(CurrentTime / Duration, 0.0f, 1.0f);

        // Calcul de la nouvelle position
        FVector NewLocation = CalculateHermite(StartPoint, StartTangent, EndPoint, EndTangent, t);

        // Appliquer le mouvement
        if (TargetActor)
        {
            TargetActor->SetActorLocation(NewLocation);
        }
        else
        {
            SetActorLocation(NewLocation);
        }

        // Fin du mouvement
        if (CurrentTime >= Duration)
        {
            bIsMoving = false;
        }
    }
}

void AHermiteMover::StartMovement()
{
    CurrentTime = 0.0f;
    bIsMoving = true;
}

void AHermiteMover::ResetMovement()
{
    bIsMoving = false;
    CurrentTime = 0.0f;
    FVector ResetLoc = StartPoint;

    if (TargetActor)
    {
        TargetActor->SetActorLocation(ResetLoc);
    }
    else
    {
        SetActorLocation(ResetLoc);
    }
}

// La formule d'Hermite Cubique
FVector AHermiteMover::CalculateHermite(FVector P0, FVector M0, FVector P1, FVector M1, float t)
{
    float t2 = t * t;
    float t3 = t2 * t;

    // Les 4 fonctions de base d'Hermite
    float H00 = 2 * t3 - 3 * t2 + 1;      // Influence de P0
    float H10 = t3 - 2 * t2 + t;          // Influence de M0 (Tangente Départ)
    float H01 = -2 * t3 + 3 * t2;         // Influence de P1
    float H11 = t3 - t2;                  // Influence de M1 (Tangente Arrivée)

    // Combinaison linéaire
    FVector Result = (H00 * P0) + (H10 * M0) + (H01 * P1) + (H11 * M1);

    return Result;
}