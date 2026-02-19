#include "CatmullMover.h"
#include "DrawDebugHelpers.h"
#include "Components/LineBatchComponent.h"

// Sets default values
ACatmullMover::ACatmullMover()
{
    PrimaryActorTick.bCanEverTick = true;
    bIsMoving = false;
    CurrentTime = 0.0f;
    PrimaryActorTick.bCanEverTick = false;

    PreviewSpline = CreateDefaultSubobject<USplineComponent>(TEXT("PreviewSpline"));
    SetRootComponent(PreviewSpline);

    PreviewSpline->bDrawDebug = true;
    PreviewSpline->SetClosedLoop(false);

    SamplesPerSegment = 16;
    bClosedLoop = false;

    // Example default control points (can be removed)
    /*Points.Add(FVector(0.f, 0.f, 0.f));
    Points.Add(FVector(200.f, 0.f, 0.f));
    Points.Add(FVector(400.f, 200.f, 0.f));
    Points.Add(FVector(600.f, 0.f, 0.f));*/

    /*
    StartTangent = FVector(0, 0, 500);
    EndTangent = FVector(0, 0, -500);

    ControlPoints.Add(FVector(0.0,0.0,0.0));
    ControlPoints.Add(FVector(1.0,0.0,0.0));
    ControlPoints.Add(FVector(2.0,0.0,0.0));
    ControlPoints.Add(FVector(3.0,0.0,0.0));*/

//#if WITH_EDITORONLY_DATA
//    if (!RootComponent)
//    {
//        RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
//    }
//
//    EditorLineBatch = CreateDefaultSubobject<ULineBatchComponent>(TEXT("CatmullEditorLineBatch"));
//    EditorLineBatch->SetupAttachment(RootComponent);
//
//    EditorLineBatch->bHiddenInGame = true;
//    EditorLineBatch->SetIsVisualizationComponent(true);
//    EditorLineBatch->bCalculateAccurateBounds = true;
//    EditorLineBatch->CastShadow = false;
//#endif

}

// Called when the game starts or when spawned
void ACatmullMover::BeginPlay()
{
    Super::BeginPlay();

    AActor* ActualTarget = TargetActor ? TargetActor : this;

    //ActivatedTriggers.Init(false, TriggerPercents.Num());
    //// En mode additif, on peut initialiser StartPoint pour aider le reset/debug
    //if (bAdditive)
    //{
    //    if (StartPoint.IsZero())
    //    {
    //        StartPoint = ActualTarget->GetActorLocation();
    //    }
    //}

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

    PreviewSpline->ClearSplinePoints(false);

    const int32 NumSegments = DebugSegments;
    if (NumSegments <= 0 || SamplesPerSegment < 2)
    {
        // Just show control points if any
        for (int32 i = 0; i < ControlPoints.Num(); ++i)
        {
            const FVector WorldPos = Transform.TransformPosition(ControlPoints[i]);
            PreviewSpline->AddSplinePoint(WorldPos, ESplineCoordinateSpace::World, false);
        }
        PreviewSpline->SetClosedLoop(bClosedLoop, false);
        PreviewSpline->UpdateSpline();
        return;
    }

    // Sample Catmull-Rom and feed into PreviewSpline
    for (int32 SegmentIndex = 0; SegmentIndex < NumSegments; ++SegmentIndex)
    {
        for (int32 SampleIndex = 0; SampleIndex < SamplesPerSegment; ++SampleIndex)
        {
            const float Alpha = static_cast<float>(SampleIndex) / (SamplesPerSegment - 1);
            const FVector LocalPos = CalculateCatmull(SegmentIndex, Alpha);
            const FVector WorldPos = Transform.TransformPosition(LocalPos);

            PreviewSpline->AddSplinePoint(WorldPos, ESplineCoordinateSpace::World, false);
        }
    }

    PreviewSpline->SetClosedLoop(bClosedLoop, false);
    PreviewSpline->UpdateSpline();


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

    
    /*if (CurrentTime >= Duration)
    {
        bIsMoving = false;
    }*/
    if (bUseEaseInOut)
    {
        t = FMath::InterpEaseInOut(0.0f, 1.0f, t, EaseExponent);
    }
    if (CurrentTime >= Duration)
    {
        CurrentTime = Duration;
        bIsMoving = false;
       // OnAnimationEnded();
    }
    const float InterpAlongSpline = CurrentTime / Duration;
    //ActualTarget->SetActorLocation(GetPositionInSpline(InterpAlongSpline));
    const int IndexSegment = FMath::FloorToInt(InterpAlongSpline * DebugSegments);
    float interSegment = (InterpAlongSpline * DebugSegments) - IndexSegment;


    if (bIsMoving) {
        FVector NewLocation = CalculateCatmull(IndexSegment, interSegment);
        ActualTarget->SetActorLocation(NewLocation);
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
        : FVector().Zero();

    ActualTarget->SetActorLocation(ResetLoc);
}

FVector ACatmullMover::CalculateCatmull(int32 SegmentIndex, float T) {
    /* FVector T1 = 0.5f * (P2 - P0);
     FVector T2 = 0.5f * (P3 - P1);

     return FMath::CubicInterp(P1, T1, P2, T2, T);*/
    int32 I0, I1, I2, I3;

    const int32 NumPoints = Points.Num();

    if (NumPoints < 4)
    {
        // Fallback: clamp indices if not enough points
        I1 = FMath::Clamp(SegmentIndex, 0, NumPoints - 1);
        I2 = FMath::Clamp(SegmentIndex + 1, 0, NumPoints - 1);
        I0 = FMath::Clamp(I1 - 1, 0, NumPoints - 1);
        I3 = FMath::Clamp(I2 + 1, 0, NumPoints - 1);
        return;
    }

    auto WrapIndex = [NumPoints, this](int32 Index) -> int32
        {
            if (bClosedLoop)
            {
                return (Index % NumPoints + NumPoints) % NumPoints;
            }
            else
            {
                return FMath::Clamp(Index, 0, NumPoints - 1);
            }
        };

    I1 = WrapIndex(SegmentIndex);
    I2 = WrapIndex(SegmentIndex + 1);
    I0 = WrapIndex(SegmentIndex - 1);
    I3 = WrapIndex(SegmentIndex + 2);


    const FVector& P0 = ControlPoints[I0];
    const FVector& P1 = ControlPoints[I1];
    const FVector& P2 = ControlPoints[I2];
    const FVector& P3 = ControlPoints[I3];

    if (T < 0.0f) T = 0.0f;
    if (T > 1.0f) T = 1.0f;

    float t2 = T * T;
    float t3 = t2 * T;

    return (P1 * 2.0f +
        (P2 - P0) * T +
        (P0 * 2.0f - P1 * 5.0f + P2 * 4.0f - P3) * t2 +
        (-P0 + P1 * 3.0f - P2 * 3.0f + P3) * t3) * 0.5f;

}
