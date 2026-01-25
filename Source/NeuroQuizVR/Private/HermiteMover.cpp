#include "HermiteMover.h"
#include "DrawDebugHelpers.h"


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

static void DrawHermiteDebug(UWorld* World, const FVector& P0, const FVector& M0, const FVector& P1, const FVector& M1, int Segments)
{
    if (!World) return;
    Segments = FMath::Max(2, Segments);

    FVector Prev = FVector::ZeroVector;

    for (int i = 0; i <= Segments; ++i)
    {
        float u = (float)i / (float)Segments;

        // Hermite (copie de CalculateHermite mais en static)
        float u2 = u * u;
        float u3 = u2 * u;

        float H00 = 2 * u3 - 3 * u2 + 1;
        float H10 = u3 - 2 * u2 + u;
        float H01 = -2 * u3 + 3 * u2;
        float H11 = u3 - u2;

        FVector Cur = (H00 * P0) + (H10 * M0) + (H01 * P1) + (H11 * M1);

        if (i > 0)
        {
            DrawDebugLine(World, Prev, Cur, FColor::Green, false, 0.0f, 0, 2.0f);
        }
        Prev = Cur;
    }

    // Debug des poignées (visuel clair)
    DrawDebugSphere(World, P0, 6.f, 8, FColor::Cyan, false, 0.0f);
    DrawDebugSphere(World, P1, 6.f, 8, FColor::Cyan, false, 0.0f);
    DrawDebugLine(World, P0, P0 + M0, FColor::Yellow, false, 0.0f, 0, 1.0f);
    DrawDebugLine(World, P1, P1 + M1, FColor::Yellow, false, 0.0f, 0, 1.0f);
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

        if (bDrawDebugCurve)
        {
            UWorld* World = GetWorld();

            if (bAdditive)
            {
                // Base (world)
                FVector Base = bBaseCaptured ? BaseLocation : ActualTarget->GetActorLocation();

                // Courbe en world : P0=Base, P1=Base+EndPoint
                FVector P0 = Base;
                FVector P1 = Base + EndPoint;

                // Handles => tangentes (vecteurs)
                // P0 handle : StartTangent est déjà en "delta" si tu le déplaces comme offset
                FVector M0 = StartTangent;

                // P1 handle : EndTangent est un handle en delta, donc vecteur relatif: (EndTangent - EndPoint)
                FVector M1 = EndTangent - EndPoint;

                DrawHermiteDebug(World, P0, M0, P1, M1, DebugSegments);
            }
            else
            {
                // Absolute : P0=StartPoint, P1=EndPoint
                FVector P0 = StartPoint;
                FVector P1 = EndPoint;

                // Handles => vecteurs
                FVector M0 = StartTangent - StartPoint;
                FVector M1 = EndTangent - EndPoint;

                DrawHermiteDebug(World, P0, M0, P1, M1, DebugSegments);
            }
        }


        FVector NewLocation;

        if (bAdditive)
        {
            if (!bBaseCaptured)
            {
                BaseLocation = ActualTarget->GetActorLocation();
                bBaseCaptured = true;
            }

            // mêmes tangentes que le debug additif
            FVector M0 = StartTangent;            // car P0 = 0
            FVector M1 = EndTangent - EndPoint;   // handle fin relatif

            FVector Delta = CalculateHermite(FVector::ZeroVector, M0, EndPoint, M1, t);
            NewLocation = BaseLocation + Delta;
        }
        else
        {
            // mêmes tangentes que le debug absolute
            FVector M0 = StartTangent - StartPoint;
            FVector M1 = EndTangent - EndPoint;

            NewLocation = CalculateHermite(StartPoint, M0, EndPoint, M1, t);
        }


        ActualTarget->SetActorLocation(NewLocation);

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