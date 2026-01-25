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

        FVector NewLocation;

        if (bAdditive)
        {
            // Si jamais StartMovement n'a pas été appelé, on capture quand même une base
            if (!bBaseCaptured)
            {
                BaseLocation = ActualTarget->GetActorLocation();
                bBaseCaptured = true;
            }

            // Hermite en OFFSET : 0 -> EndPoint (EndPoint devient un déplacement final)
            FVector Delta = CalculateHermite(
                FVector::ZeroVector,
                StartTangent,
                EndPoint,
                EndTangent,
                t
            );

            NewLocation = BaseLocation + Delta;
        }
        else
        {
            // Mode ABSOLU : StartPoint -> EndPoint (positions world)
            NewLocation = CalculateHermite(
                StartPoint,
                StartTangent,
                EndPoint,
                EndTangent,
                t
            );
        }



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

    FVector ResetLoc;

    if (bAdditive)
    {
        ResetLoc = bBaseCaptured ? BaseLocation : ActualTarget->GetActorLocation();
    }
    else
    {
        ResetLoc = StartPoint;
    }


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