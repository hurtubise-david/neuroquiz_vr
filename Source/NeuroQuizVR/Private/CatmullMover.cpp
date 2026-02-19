#include "CatmullMover.h"
#include "DrawDebugHelpers.h"
#include "Components/LineBatchComponent.h"

// Sets default values
ACatmullMover::ACatmullMover()
{
    PrimaryActorTick.bCanEverTick = true;
    bIsMoving = false;
    CurrentTime = 0.0f;
    //PrimaryActorTick.bCanEverTick = false;

    PreviewSpline = CreateDefaultSubobject<USplineComponent>(TEXT("PreviewSpline"));
    SetRootComponent(PreviewSpline);

    PreviewSpline->bDrawDebug = true;
    PreviewSpline->SetClosedLoop(false);

    SamplesPerSegment = 16;
    bClosedLoop = false;

}

// Called when the game starts or when spawned
void ACatmullMover::BeginPlay()
{
    Super::BeginPlay();

    AActor* ActualTarget = TargetActor ? TargetActor : this;

    StartMovement();

}


#if WITH_EDITOR
void ACatmullMover::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    AActor* ActualTarget = TargetActor ? TargetActor : this;
    PreviewSpline->ClearSplinePoints(false);
    FVector Base = bBaseCaptured ? BaseLocation : ActualTarget->GetActorLocation();
    const int32 NumSegments = DebugSegments;
    if (NumSegments <= 0 || SamplesPerSegment < 2)
    {
        for (int32 i = 0; i < Points.Num(); ++i)
        {
            const FVector WorldPos = Transform.TransformPosition(Points[i] + Base);
            PreviewSpline->AddSplinePoint(WorldPos, ESplineCoordinateSpace::World, false);
        }
        PreviewSpline->SetClosedLoop(bClosedLoop, false);
        PreviewSpline->UpdateSpline();
        return;
    }

    for (int32 SegmentIndex = 0; SegmentIndex < NumSegments; ++SegmentIndex)
    {
        for (int32 SampleIndex = 0; SampleIndex < SamplesPerSegment; ++SampleIndex)
        {
            const float Alpha = static_cast<float>(SampleIndex) / (SamplesPerSegment - 1);
            const FVector LocalPos = CalculateCatmull(SegmentIndex, Alpha) + Base;
            const FVector WorldPos = Transform.TransformPosition(LocalPos);

            PreviewSpline->AddSplinePoint(WorldPos, ESplineCoordinateSpace::World, false);
        }
    }

    PreviewSpline->SetClosedLoop(bClosedLoop, false);
    PreviewSpline->UpdateSpline();

#endif
}
//#endif

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

    if (CurrentTime >= Duration)
    {
        CurrentTime = Duration;
        bIsMoving = false;
       // OnAnimationEnded();
    }
    if (bIsMoving) {
        const float InterpAlongSpline = CurrentTime / Duration;

        const int32 NumSegments = DebugSegments;

        float GlobalT = InterpAlongSpline * NumSegments;

        int32 SegmentIndex = FMath::Clamp(FMath::FloorToInt(GlobalT), 0, NumSegments - 1);

        float LocalT = GlobalT - SegmentIndex;

        FVector NewLocation = CalculateCatmull(SegmentIndex, LocalT);

        ActualTarget->SetActorLocation(NewLocation);
    }
    
}

void ACatmullMover::StartMovement()
{
    CurrentTime = 0.0f;
    bIsMoving = true;
}


void ACatmullMover::ResetMovement()
{
    bIsMoving = false;
    CurrentTime = 0.0f;

}

FVector ACatmullMover::CalculateCatmull(int32 SegmentIndex, float T) {
    int32 I0, I1, I2, I3;

    const int32 NumPoints = Points.Num();

    if (NumPoints < 4)
    {
        I1 = FMath::Clamp(SegmentIndex, 0, NumPoints - 1);
        I2 = FMath::Clamp(SegmentIndex + 1, 0, NumPoints - 1);
        I0 = FMath::Clamp(I1 - 1, 0, NumPoints - 1);
        I3 = FMath::Clamp(I2 + 1, 0, NumPoints - 1);
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


    const FVector& P0 = Points[I0];
    const FVector& P1 = Points[I1];
    const FVector& P2 = Points[I2];
    const FVector& P3 = Points[I3];

    if (T < 0.0f) T = 0.0f;
    if (T > 1.0f) T = 1.0f;

    float t2 = T * T;
    float t3 = t2 * T;

    return (P1 * 2.0f +
        (P2 - P0) * T +
        (P0 * 2.0f - P1 * 5.0f + P2 * 4.0f - P3) * t2 +
        (-P0 + P1 * 3.0f - P2 * 3.0f + P3) * t3) * 0.5f;

}
