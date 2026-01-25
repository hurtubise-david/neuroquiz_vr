#include "HermiteMover.h"
#include "DrawDebugHelpers.h"
#include "Components/LineBatchComponent.h"



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


#if WITH_EDITOR
void AHermiteMover::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

#if WITH_EDITORONLY_DATA
    if (!EditorLineBatch)
        return;

    // Si debug off -> on nettoie et on sort
    if (!bDrawDebugCurve || !bDrawDebugInEditor)
    {
        EditorLineBatch->Flush();
        return;
    }

    // IMPORTANT : on flush avant de redessiner => aucune duplication
    EditorLineBatch->Flush();

    AActor* ActualTarget = TargetActor ? TargetActor : this;

    // Construire P0/P1/M0/M1 (comme dans ton Tick)
    FVector P0, P1, M0, M1;

    if (bAdditive)
    {
        FVector Base = bBaseCaptured ? BaseLocation : ActualTarget->GetActorLocation();
        P0 = Base;
        P1 = Base + EndPoint;

        M0 = StartTangent;
        M1 = EndTangent - EndPoint;
    }
    else
    {
        P0 = StartPoint;
        P1 = EndPoint;

        M0 = StartTangent - StartPoint;
        M1 = EndTangent - EndPoint;
    }

    const int Segments = FMath::Max(2, DebugSegments);

    // Dessin de la "courbe" (en fait segments successifs)
    FVector Prev = P0;
    for (int i = 1; i <= Segments; ++i)
    {
        float u = (float)i / (float)Segments;

        // Hermite via ta fonction (plus clean que dupliquer la formule)
        FVector Cur = CalculateHermite(P0, M0, P1, M1, u);

        // 0.0f lifetime => "persistant" tant que tu flush pas
        const float LifeTime = 0.0f;
        const float Thickness = 3.0f;
        const uint8 DepthPriority = 0;

        EditorLineBatch->BatchedLines.Add(
            FBatchedLine(Prev, Cur, FLinearColor::Green, LifeTime, Thickness, DepthPriority)
        );

        Prev = Cur;
    }

    // Force refresh (souvent pas nécessaire, mais safe)
    EditorLineBatch->MarkRenderStateDirty();
#endif
}
#endif



void AHermiteMover::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    AActor* ActualTarget = TargetActor ? TargetActor : this;


    // 1) DEBUG (si activé)
    if (bDrawDebugCurve)
    {
        UWorld* World = GetWorld();

        if (bAdditive)
        {
            // Base (world) : si pas capturé, on prend la position actuelle
            FVector Base = bBaseCaptured ? BaseLocation : ActualTarget->GetActorLocation();

            FVector P0 = Base;
            FVector P1 = Base + EndPoint;

            FVector M0 = StartTangent;          // P0 = 0 en additif
            FVector M1 = EndTangent - EndPoint; // handle fin relatif

            DrawHermiteDebug(World, P0, M0, P1, M1, DebugSegments);
        }
        else
        {
            FVector P0 = StartPoint;
            FVector P1 = EndPoint;

            FVector M0 = StartTangent - StartPoint;
            FVector M1 = EndTangent - EndPoint;

            DrawHermiteDebug(World, P0, M0, P1, M1, DebugSegments);
        }
    }


    // 2) SI PAS EN MOUVEMENT, ON SORT
    if (!bIsMoving)
        return;


    // 3) MOUVEMENT
    CurrentTime += DeltaTime;

    float t = FMath::Clamp(CurrentTime / Duration, 0.0f, 1.0f);

    if (bUseEaseInOut)
    {
        t = FMath::InterpEaseInOut(0.0f, 1.0f, t, EaseExponent);
    }

    FVector NewLocation;

    if (bAdditive)
    {
        if (!bBaseCaptured)
        {
            BaseLocation = ActualTarget->GetActorLocation();
            bBaseCaptured = true;
        }

        FVector M0 = StartTangent;
        FVector M1 = EndTangent - EndPoint;

        FVector Delta = CalculateHermite(FVector::ZeroVector, M0, EndPoint, M1, t);
        NewLocation = BaseLocation + Delta;
    }
    else
    {
        FVector M0 = StartTangent - StartPoint;
        FVector M1 = EndTangent - EndPoint;

        NewLocation = CalculateHermite(StartPoint, M0, EndPoint, M1, t);
    }

    ActualTarget->SetActorLocation(NewLocation);

    if (CurrentTime >= Duration)
    {
        bIsMoving = false;
    }
}


AHermiteMover::AHermiteMover()
{
    PrimaryActorTick.bCanEverTick = true;
    bIsMoving = false;
    CurrentTime = 0.0f;

    StartTangent = FVector(0, 0, 500);
    EndTangent = FVector(0, 0, -500);

#if WITH_EDITORONLY_DATA
    // Root garanti
    if (!RootComponent)
    {
        RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    }

    EditorLineBatch = CreateDefaultSubobject<ULineBatchComponent>(TEXT("HermiteEditorLineBatch"));
    EditorLineBatch->SetupAttachment(RootComponent);

    EditorLineBatch->bHiddenInGame = true;
    EditorLineBatch->SetIsVisualizationComponent(true);
    EditorLineBatch->bCalculateAccurateBounds = true;
    EditorLineBatch->CastShadow = false;
#endif
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