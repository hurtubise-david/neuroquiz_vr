// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Col_CatmullMover.generated.h"

UCLASS()
class NEUROQUIZVR_API ACol_CatmullMover : public AActor
{
	GENERATED_BODY()
	
public:	
	ACol_CatmullMover();
	void InitSpline();
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "Neuro|Catmull Spline")
	void StartMovement();
	
	UFUNCTION(BlueprintCallable, Category = "Neuro|Catmull Spline")
	void StopMovement();
	
	UFUNCTION(BlueprintCallable, Category = "Neuro|Catmull Spline")
	void ToggleMovement();

	UFUNCTION(BlueprintCallable, Category = "Neuro|Catmull Spline")
	void ResetMovement();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Neuro|Catmull Spline")
	void OnAnimationEnded();
	virtual void OnAnimationEnded_Implementation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Neuro|Catmull Spline")
	void OnSplinePercentReached(float InterpAlongSpline);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Neuro|Catmull Spline")
	TArray<float> TriggerPercents;


protected:
	virtual void BeginPlay() override;
	FVector GetPositionInSpline(float InterpAlongSpline);
	FVector GetPositionInCurrentSegment(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T) const;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Neuro|Catmull Spline", meta = (MakeEditWidget = true))
	TArray<FVector> Points;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuro|Catmull Spline")
	float Duration = 3.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuro|Catmull Spline")
	bool bUseEaseInOut = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuro|Catmull Spline", meta = (ClampMin = "0.1", ClampMax = "8.0"))
	float EaseExponent = 2.0f;
	
	UPROPERTY(EditAnywhere, Category = "Neuro|Catmull Spline")
	float Tension = 0.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuro|Catmull Spline")
	TArray<TObjectPtr<AActor>> TargetActors;
	
	// --- Debugs lines ---
	UPROPERTY(EditAnywhere, Category = "Neuro|Catmull Spline|Debug")
	bool bDrawDebugCurve = true;

	UPROPERTY(EditAnywhere, Category = "Neuro|Catmull Spline|Debug", meta = (ClampMin = "4", ClampMax = "200"))
	int DebugSegments = 30;

	UPROPERTY(EditAnywhere, Category = "Neuro|Catmull Spline|Debug")
	bool bDrawDebugInEditor = true;

private:	
	
	TArray<FVector> PointsWithControls;
	
	int32 NumSegments;

	bool bIsMoving;
	
	float CurrentTime;
	
	TArray<bool> ActivatedTriggers;
	
#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	ULineBatchComponent* EditorLineBatch = nullptr;
#endif
	
};
