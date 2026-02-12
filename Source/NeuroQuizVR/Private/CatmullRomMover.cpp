// Fill out your copyright notice in the Description page of Project Settings.


#include "CatmullRomMover.h"
#include "DrawDebugHelpers.h"
#include "Components/LineBatchComponent.h"

CatmullRomMover::CatmullRomMover()
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

    EditorLineBatch = CreateDefaultSubobject<ULineBatchComponent>(TEXT("CatmullRomEditorLineBatch"));
    EditorLineBatch->SetupAttachment(RootComponent);

    EditorLineBatch->bHiddenInGame = true;
    EditorLineBatch->SetIsVisualizationComponent(true);
    EditorLineBatch->bCalculateAccurateBounds = true;
    EditorLineBatch->CastShadow = false;
#endif
}

void ACatmullRomMover::BeginPlay()
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

static void DrawCatmullRomDebug(UWorld* World, const FVector& P0, const FVector& M0, const FVector& P1, const FVector& M1, int Segments)
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
void ACatmullRomMover::OnConstruction(const FTransform& Transform)
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



void ACatmullRomMover::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    AActor* ActualTarget = TargetActor ? TargetActor : this;

    // 1) SI MOINS QUE 2 POINTS DE CONTROLE, ON SORT
    if (bControlPoints.Num() < 2)
        return;

    // 2) SI PAS EN MOUVEMENT, ON SORT
    if (!bIsMoving)
        return;


    // 3) MOUVEMENT
    CurrentTime += DeltaTime;

    float t = FMath::Clamp(CurrentTime / Duration, 0.0f, 1.0f);
    int nbSegments = bControlPoints.Num() - 1;
    int currentSegment = FMath::Clamp(FMath::FloorToInt(t * nbSegments), 0, nbSegments - 1)

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

        FVector Delta = CalculateSegmentWithHermit(currentSegment, t);
        NewLocation = BaseLocation + Delta;
    }
    else
    {
        NewLocation = CalculateSegmentWithHermit(currentSegment, t);
    }

    ActualTarget->SetActorLocation(NewLocation);

    if (CurrentTime >= Duration)
    {
        bIsMoving = false;
    }
}

void ACatmullRomMover::StartMovement()
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


void ACatmullRomMover::ResetMovement()
{
    bIsMoving = false;
    CurrentTime = 0.0f;

    AActor* ActualTarget = TargetActor ? TargetActor : this;

    FVector ResetLoc = bAdditive
        ? (bBaseCaptured ? BaseLocation : ActualTarget->GetActorLocation())
        : StartPoint;

    ActualTarget->SetActorLocation(ResetLoc);
}

FVector CalculateTangent(int i) {
    int size = bControlPoints.Num();
    
    // 1) SI MOINS QUE 2 POINTS DE CONTROLE, ON SORT
    if (size < 2)
        return;

    float scale = (1.0f - bTension) * 0.5f;

    if (i == 0) {
        // Premier point
        return scale * (bControlPoints[1] - bControlPoints[0]);
    }
    else if (i == size - 1) {
        // Dernier point
        return scale * (bControlPoints[size - 1] - bControlPoints[size - 2]);
    }
    else {
        // Autres points
        return scale * (bControlPoints[i + 1] - bControlPoints[i - 1]);
    }
}

FVector CalculateSegmentWithHermit(int i, float t) {
    const FVector& p0 = bControlPoints[i];
    const FVector& p1 = bControlPoints[i + 1];

    FVector t0 = CalculateTangent(i);
    FVector t1 = CalculateTangent(i + 1);

    return HermitMover.CalculateHermite(p0, t0, p1, t1, f);
}