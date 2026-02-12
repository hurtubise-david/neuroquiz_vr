// Fill out your copyright notice in the Description page of Project Settings.

#include "Collaboration/Col_CatmullMover.h"

ACol_CatmullMover::ACol_CatmullMover()
{
	PrimaryActorTick.bCanEverTick = true;
	bIsMoving = false;
	CurrentTime = 0.0f;
}

void ACol_CatmullMover::BeginPlay()
{
	Super::BeginPlay();
	ActualTarget = TargetActor ? TargetActor : this;
	ActivatedTriggers.Init(false, TriggerPercents.Num());
	InitSpline();
}

void ACol_CatmullMover::InitSpline()
{
	int NumPoints = Points.Num();
	
	if (NumPoints < 2)	return;
	
	//Pour l'instant je fait juste dupliquer le premier et le dernier point 
	//pour garantir qu'on a toujours 4 points pour le calcul de la spline
	//Je sais pas trop si on veut pouvoir bouger les points de controle
	PointsWithControls.Reset();
	PointsWithControls.Add(Points[0]); 
	PointsWithControls.Append(Points);
	PointsWithControls.Add(Points[NumPoints - 1]); 
	
	NumSegments = NumPoints - 1;
}



void ACol_CatmullMover::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!bIsMoving) return;
	
	CurrentTime += DeltaTime;

	
	if (CurrentTime >= Duration)
	{
		CurrentTime = Duration;
		bIsMoving = false;
		OnAnimationEnded();
	}
	
	const float InterpAlongSpline = CurrentTime / Duration;

	for (int i = 0; i < TriggerPercents.Num(); ++i)
	{	
		if (!ActivatedTriggers[i] && InterpAlongSpline >= (TriggerPercents[i]/100.0f))
		{
			ActivatedTriggers[i] = true;
			OnSplinePercentReached(InterpAlongSpline);
		}
	}
	
	ActualTarget->SetActorLocation(GetPositionInSpline(InterpAlongSpline));
}

#if WITH_EDITOR
void ACol_CatmullMover::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
	
	const UWorld* World = GetWorld();
	check(World);
    
    if (!bDrawDebugCurve || !bDrawDebugInEditor)
    {
    	FlushPersistentDebugLines(World);
        return;
    }
    
    InitSpline();
    
    if (NumSegments == 0)
        return;
    
    FlushPersistentDebugLines(World);
    
	FTransform ActorTransform = TargetActor
	? TargetActor->GetActorTransform()
	: GetActorTransform();

    
    for (int32 CurrentSegment = 0; CurrentSegment < NumSegments; CurrentSegment++)
    {
        FVector P0 = PointsWithControls[CurrentSegment];
        FVector P1 = PointsWithControls[CurrentSegment + 1];
        FVector P2 = PointsWithControls[CurrentSegment + 2];
        FVector P3 = PointsWithControls[CurrentSegment + 3];
        
        FVector PrevPoint = ActorTransform.TransformPosition(P1);
        
        for (int32 i = 1; i <= DebugSegments; i++)
        {
            const float InterpAlongSegment = static_cast<float>(i) / static_cast<float>(DebugSegments);
            FVector LocalPos = GetPositionInCurrentSegment(P0, P1, P2, P3, InterpAlongSegment);
            FVector WorldPos = ActorTransform.TransformPosition(LocalPos);
            
            DrawDebugLine(World,PrevPoint,WorldPos,FColor::Green,true,-1.0f,0,3.0f);
            
            PrevPoint = WorldPos;
        }
    }
}
#endif

FVector ACol_CatmullMover::GetPositionInSpline(const float InterpAlongSpline)
{
	const float SegmentFloat = InterpAlongSpline * NumSegments;
	const int CurrentSegment = FMath::FloorToInt(SegmentFloat);
	const float InterpAlongSegment = SegmentFloat - CurrentSegment;

	const FVector P0 = PointsWithControls[CurrentSegment];
	const FVector P1 = PointsWithControls[CurrentSegment + 1];
	const FVector P2 = PointsWithControls[CurrentSegment + 2];
	const FVector P3 = PointsWithControls[CurrentSegment + 3];
	
	return GetPositionInCurrentSegment(P0,P1,P2,P3,InterpAlongSegment);
}

FVector ACol_CatmullMover::GetPositionInCurrentSegment(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, const float T) const
{
	const float T2 = T * T;
	const float T3 = T2 * T;    
	
	return 
		Tension * 
		(
			2.0f * P1 +                                      
			(-P0 + P2) * T +                                   
			(2.0f * P0 - 5.0f * P1 + 4.0f * P2 - P3) * T2 +   
			(-P0 + 3.0f * P1 - 3.0f * P2 + P3) * T3           
		);
}


void ACol_CatmullMover::StartMovement()
{
	CurrentTime = 0.0f;
	bIsMoving = true;
}

void ACol_CatmullMover::StopMovement()
{
	bIsMoving = false;
}

void ACol_CatmullMover::ToggleMovement()
{
	bIsMoving = !bIsMoving;
}

void ACol_CatmullMover::ResetMovement()
{
	CurrentTime = 0.0f;
}

void ACol_CatmullMover::OnAnimationEnded_Implementation()
{
	
}

