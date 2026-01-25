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

    AActor* ActualTarget = TargetActor ? TargetActor : this;

    // En mode additif, on peut initialiser StartPoint pour aider le reset/debug
    if (bAdditive)
    {
        if (StartPoint.IsZero())
        {
            StartPoint = ActualTarget->GetActorLocation();
        }
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

        if (bUseEaseInOut)
        {
            t = FMath::InterpEaseInOut(0.0f, 1.0f, t, EaseExponent);
        }


        // Calcul de la nouvelle position
        AActor* ActualTarget = TargetActor ? TargetActor : this;

        // (optionnel) ease in/out ici si tu l’as ajouté

        FVector NewLocation;

        if (bAdditive)
        {
            if (!bBaseCaptured)
            {
                BaseLocation = ActualTarget->GetActorLocation();
                bBaseCaptured = true;
            }

            FVector Delta = CalculateHermite(FVector::ZeroVector, StartTangent, EndPoint, EndTangent, t);
            NewLocation = BaseLocation + Delta;
        }
        else
        {
            // ABSOLUTE => suit les widgets WORLD
            NewLocation = CalculateHermite(StartPoint, StartTangent, EndPoint, EndTangent, t);
        }

        ActualTarget->SetActorLocation(NewLocation);




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

    AActor* ActualTarget = TargetActor ? TargetActor : this;

    if (bAdditive)
    {
        BaseLocation = ActualTarget->GetActorLocation();
        bBaseCaptured = true;
    }
    else
    {
        bBaseCaptured = false; 
    }
}

void AHermiteMover::ResetMovement()
{
    bIsMoving = false;
    CurrentTime = 0.0f;

    AActor* ActualTarget = TargetActor ? TargetActor : this;

    FVector ResetLoc = bAdditive
        ? (bBaseCaptured ? BaseLocation : ActualTarget->GetActorLocation())
        : StartPoint;

    ActualTarget->SetActorLocation(ResetLoc);
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