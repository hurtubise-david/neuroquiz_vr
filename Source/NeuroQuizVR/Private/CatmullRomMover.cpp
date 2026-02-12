#include "CatmullRomMover.h"
#include "HermiteMover.h"
#include "DrawDebugHelpers.h"
#include "Components/LineBatchComponent.h"

ACatmullRomMover::ACatmullRomMover()
{
    PrimaryActorTick.bCanEverTick = true;
    bIsMoving = false;
    CurrentTime = 0.0f;

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
        if (BaseLocation.IsZero())
        {
            BaseLocation = ActualTarget->GetActorLocation();
        }
    }

}

static void DrawCatmullRomDebug(UWorld* World, const FVector& P0, const FVector& M0, const FVector& P1, const FVector& M1, int Segments)
{

}


#if WITH_EDITOR
void ACatmullRomMover::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

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
    int currentSegment = FMath::Clamp(FMath::FloorToInt(t * nbSegments), 0, nbSegments - 1);

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
        : BaseLocation;

    ActualTarget->SetActorLocation(ResetLoc);
}

FVector ACatmullRomMover::CalculateTangent(int i) {
    int size = bControlPoints.Num();
    
    // 1) SI MOINS QUE 2 POINTS DE CONTROLE, ON SORT
    if (size < 2)
        return FVector::ZeroVector;

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

FVector ACatmullRomMover::CalculateSegmentWithHermit(int i, float t) {
    const FVector& p0 = bControlPoints[i];
    const FVector& p1 = bControlPoints[i + 1];

    FVector t0 = CalculateTangent(i);
    FVector t1 = CalculateTangent(i + 1);

    float t2 = t * t;
    float t3 = t2 * t;

    // Les 4 fonctions de base d'Hermite
    float H00 = 2 * t3 - 3 * t2 + 1;      // Influence de p0
    float H10 = t3 - 2 * t2 + t;          // Influence de t0 (Tangente Départ)
    float H01 = -2 * t3 + 3 * t2;         // Influence de p1
    float H11 = t3 - t2;                  // Influence de t1 (Tangente Arrivée)

    // Combinaison linéaire
    FVector Result = (H00 * p0) + (H10 * t0) + (H01 * p1) + (H11 * t1);

    return Result;
}