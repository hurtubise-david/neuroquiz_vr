// Fill out your copyright notice in the Description page of Project Settings.

#include "CatmullRomMover.h"

ACatmullRomMover::ACatmullRomMover()
{
	PrimaryActorTick.bCanEverTick = true;
	bIsMoving = false;
	CurrentTime = 0.0f;
}

void ACatmullRomMover::BeginPlay()
{
	if (Points.Num() < 4) return;

	AActor* Target = TargetActor ? TargetActor : this;

	BaseLocation = Target->GetActorLocation();
	CurrentTime = 0.0f;
	bIsMoving = true;
}

void ACatmullRomMover::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsMoving && Points.Num() >= 4)
	{
		CurrentTime += DeltaTime;
		float alpha = FMath::Clamp(CurrentTime / Duration, 0.0f, 1.0f);

		FVector newLocation = CalculateCatmullRom(alpha, Points);

		AActor* target = TargetActor ? TargetActor : this;
		target->SetActorLocation(BaseLocation + newLocation);

		if (alpha >= 1.0f) 
			bIsMoving = false;
	}
}

FVector ACatmullRomMover::CalculateCatmullRom(float alpha, const TArray<FVector> points)
{
	std::size_t i = FMath::FloorToInt32(alpha * (float)points.Num() - 1);
	i = FMath::Clamp(i, 0, Points.Num() - 2);

	float t = alpha * (float)points.Num() - 1 - i;

	float t2 = t * t;
	float t3 = t2 * t;
	FVector4 v1{ t3, t2, t, 1.0f };

	auto p0 = points[FMath::Max<std::size_t>(0, i - 1)];
	auto p1 = points[i];
	auto p2 = points[FMath::Min<std::size_t>(points.Num() - 1, i + 1)];
	auto p3 = points[FMath::Min<std::size_t>(points.Num() - 1, i + 2)];

	return 0.5f *
		((p1 * 2.0f) +
			(-p0 + p2) * t +
			(2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

