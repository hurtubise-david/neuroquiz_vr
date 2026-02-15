#include "CatmullMover.h"
#include "DrawDebugHelpers.h"
#include "Components/LineBatchComponent.h"

// Sets default values
ACatmullMover::ACatmullMover()
{
    PrimaryActorTick.bCanEverTick = true;
    bIsMoving = false;
    CurrentTime = 0.0f;

    StartTangent = FVector(0, 0, 500);
    EndTangent = FVector(0, 0, -500);

#if WITH_EDITORONLY_DATA
    if (!RootComponent)
    {
        RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    }

    EditorLineBatch = CreateDefaultSubobject<ULineBatchComponent>(TEXT("CatmullEditorLineBatch"));
    EditorLineBatch->SetupAttachment(RootComponent);

    EditorLineBatch->bHiddenInGame = true;
    EditorLineBatch->SetIsVisualizationComponent(true);
    EditorLineBatch->bCalculateAccurateBounds = true;
    EditorLineBatch->CastShadow = false;
#endif

}

// Called when the game starts or when spawned
void ACatmullMover::BeginPlay()
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

void ACatmullMover::GetTangent(TArray<FVector>& points, float tension) {
    const int32 num = points.Num();

    for (int32 i = 0; i < num; i++)
    {
        TangentPoints[i] = tension * (points[i + 1] - points[i - 1]);
    }

}

#if WITH_EDITOR
void ACatmullMover::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

#if WITH_EDITORONLY_DATA
    if (EditorLineBatch && !EditorLineBatch->IsRegistered())
    {
        EditorLineBatch->RegisterComponent();
    }
#endif

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
    //FVector Prev = P0;
    Points[0] = P0;
    FVector Prev = Points[0];

    for (int i = 1; i <= Segments; ++i)
    {
        FVector Cur = FVector();
        float u = (float)i / (float)Segments;
        if (i == 1) {
            M0 = CalculateCatmullRomTangent(Prev, Prev);
            TangentsList.Add(M0);
            M1 = CalculateCatmullRomTangent(Prev, Points[i + 1]);
            TangentsList.Add(M1);
            Cur = CalculateHermite(Prev, M0, Points[i], M1, u);
        }
        else if (i == Points.Num() - 1) {
            M0 = TangentsList[i - 1];
            M1 = CalculateCatmullRomTangent(Prev, Points[i + 1]);
            TangentsList[i] = M1;
            Cur = CalculateHermite(Prev, M0, Points[i], M1, u);
        }
        else {
            M0 = TangentsList[i - 1];
            M1 = CalculateCatmullRomTangent(Prev, Points[i + 1]);
            TangentsList.Add(M1);
            Cur = CalculateHermite(Prev, M0, Points[i], M1, u);
        }
        // Hermite via ta fonction (plus clean que dupliquer la formule)
        //FVector Cur = CalculateHermite(P0, M0, P1, M1, u);

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

// Called every frame
void ACatmullMover::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    AActor* ActualTarget = TargetActor ? TargetActor : this;


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

        FVector Delta = CalculateCatmull(FVector::ZeroVector, M0, EndPoint, M1, t);
        NewLocation = BaseLocation + Delta;
    }
    else
    {
        FVector M0 = StartTangent - StartPoint;
        FVector M1 = EndTangent - EndPoint;

        NewLocation = CalculateCatmull(StartPoint, M0, EndPoint, M1, t);
    }

    ActualTarget->SetActorLocation(NewLocation);

    if (CurrentTime >= Duration)
    {
        bIsMoving = false;
    }
}

void ACatmullMover::StartMovement()
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


void ACatmullMover::ResetMovement()
{
    bIsMoving = false;
    CurrentTime = 0.0f;

    AActor* ActualTarget = TargetActor ? TargetActor : this;

    FVector ResetLoc = bAdditive
        ? (bBaseCaptured ? BaseLocation : ActualTarget->GetActorLocation())
        : StartPoint;

    ActualTarget->SetActorLocation(ResetLoc);
}

FVector ACatmullMover::CalculateCatmull(FVector P0, FVector P1, FVector P2, FVector P3, float T) {
    FVector T1 = 0.5f * (P2 - P0);
    FVector T2 = 0.5f * (P3 - P1);

    return FMath::CubicInterp(P1, T1, P2, T2, T);

}
// La formule d'Hermite Cubique
FVector ACatmullMover::CalculateHermite(FVector P0, FVector M0, FVector P1, FVector M1, float t)
{
    float t2 = t * t;
    float t3 = t2 * t;

    // Les 4 fonctions de base d'Hermite
    float H00 = 2 * t3 - 3 * t2 + 1;      // Influence de P0
    float H10 = t3 - 2 * t2 + t;          // Influence de M0 (Tangente Départ)

    //FVector midPoint = (P1 - P0) / 2;

    float H01 = -2 * t3 + 3 * t2;         // Influence de P1
    float H11 = t3 - t2;                  // Influence de M1 (Tangente Arrivée)

    // Combinaison linéaire
    FVector Result = (H00 * P0) + (H10 * M0) + (H01 * P1) + (H11 * M1);

    return Result;
}
//FVector AHermiteMover::CalculateCatmullRom(
//    FVector Pprev, FVector P0, FVector P1, FVector Pnext, float t)
//{
//    // Catmull–Rom tangents
//    FVector M0 = 0.5f * (P1 - Pprev);
//    FVector M1 = 0.5f * (Pnext - P0);
//
//    // Use your Hermite function
//    return CalculateHermite(P0, M0, P1, M1, t);
//}
FVector  ACatmullMover::CalculateCatmullRomTangent(FVector P0, FVector P1) {
    FVector tangent = 0.5f * (P1 - P0);
    return tangent;
}
