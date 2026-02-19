#include "CatmullRomMover.h"
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

	EditorLineBatch->Flush();

	if (!bDrawDebugCurve || Points.Num() < 4)
		return;

	const int Segments = FMath::Max(2, DebugSegments);
	FVector ActorLocation = GetActorLocation();
	FVector Prev = ActorLocation + Points[0];

	for (int i = 1; i <= Segments; ++i)
	{
		float alpha = (float)i / (float)Segments;
		FVector Cur = ActorLocation + CalculateCatmullRom(alpha, Points);

		EditorLineBatch->BatchedLines.Add(
			FBatchedLine(Prev, Cur, FLinearColor::Green, 0.0f, 3.0f, 0)
		);

		Prev = Cur;
	}

	EditorLineBatch->MarkRenderStateDirty();
#endif
}
#endif

void ACatmullRomMover::BeginPlay()
{
	Super::BeginPlay();
}

void ACatmullRomMover::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsMoving || Points.Num() < 4)
		return;

	CurrentTime += DeltaTime;
	float Alpha = FMath::Clamp(CurrentTime / Duration, 0.0f, 1.0f);

	FVector NewLocation = CalculateCatmullRom(Alpha, Points);

	AActor* target = TargetActor ? TargetActor : this;
	target->SetActorLocation(BaseLocation + NewLocation);

	if (Alpha >= 1.0f)
		bIsMoving = false;
}

void ACatmullRomMover::StartMovement()
{
	if (Points.Num() < 4)
		return;

	AActor* Target = TargetActor ? TargetActor : this;

	if (!bBaseCaptured)
	{
		BaseLocation = Target->GetActorLocation();
		bBaseCaptured = true;
	}

	CurrentTime = 0.0f;
	bIsMoving = true;
}

void ACatmullRomMover::ResetMovement()
{
	bIsMoving = false;
	CurrentTime = 0.0f;

	AActor* Target = TargetActor ? TargetActor : this;

	if (bBaseCaptured)
	{
		Target->SetActorLocation(BaseLocation);
	}
}

FVector ACatmullRomMover::CalculateCatmullRom(float alpha, const TArray<FVector> points)
{
	if (points.Num() < 4) 
		return FVector::ZeroVector;

	float segmentFloat = alpha * (float)(points.Num() - 1);
	int32 i = FMath::FloorToInt32(segmentFloat);

	i = FMath::Clamp(i, 0, points.Num() - 2);

	float t = segmentFloat - i;

	float t2 = t * t;
	float t3 = t2 * t;

	FVector p0 = points[FMath::Max(0, i - 1)];
	FVector p1 = points[i];
	FVector p2 = points[i + 1];
	FVector p3 = points[FMath::Min(points.Num() - 1, i + 2)];

	return 0.5f * (
		(p1 * 2.0f) +
		(-p0 + p2) * t +
		(2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
		(-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
		);
}

