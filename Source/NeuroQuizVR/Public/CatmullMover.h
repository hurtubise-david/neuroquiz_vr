

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CatmullMover.generated.h"

class ULineBatchComponent;

UCLASS()
class NEUROQUIZVR_API ACatmullMover : public AActor
{
	GENERATED_BODY()
	
public:	
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CatmullRom")
    TArray<FVector> ControlPoints;
    TArray<FVector> TangentPoints;

	// Sets default values for this actor's properties
	ACatmullMover();

protected:
    UPROPERTY(Transient)
    TArray<FVector> Tangents;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

    // --- Configuration ---

   // Le point de départ (P0)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull Spline", meta = (MakeEditWidget = true))
    FVector StartPoint;

    // La tangente au départ (M0) - C'est la vitesse de départ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull Spline", meta = (MakeEditWidget = true))
    FVector StartTangent;

    // Le point d'arrivée (P1)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull Spline", meta = (MakeEditWidget = true))
    FVector EndPoint;

    // La tangente à l'arrivée (M1) - C'est la vitesse d'arrivée
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull Spline", meta = (MakeEditWidget = true))
    FVector EndTangent;

    // Mode additif : EndPoint est interprété comme un OFFSET (Delta) plutôt qu'un point world
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull Spline")
    bool bAdditive = true;

    // --- Ease In / Ease Out (timing) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull Spline|Ease")
    bool bUseEaseInOut = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull Spline|Ease", meta = (ClampMin = "0.1", ClampMax = "8.0"))
    float EaseExponent = 2.0f;

    // --- Debugs lines ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull Spline|Debug")
    bool bDrawDebugCurve = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull Spline|Debug", meta = (ClampMin = "4", ClampMax = "200"))
    int DebugSegments = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull Spline|Debug")
    bool bDrawDebugInEditor = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull Spline|Debug", meta = (ClampMin = "0.01", ClampMax = "10.0"))
    float DebugLifetime = 0.25f;


    // Durée du mouvement en secondes
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull Spline")
    float Duration = 3.0f;

    // L'objet à déplacer (si vide, l'acteur se déplace lui-même)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catmull Spline")
    AActor* TargetActor;

    // --- Contrôles ---

    UFUNCTION(BlueprintCallable, Category = "Catmull Spline")
    void StartMovement();

    UFUNCTION(BlueprintCallable, Category = "Catmull Spline")
    void ResetMovement();

private:
	float CurrentTime;
	bool bIsMoving;

	// capturée au StartMovement
	FVector BaseLocation;
	bool bBaseCaptured = false;

    void GetTangent(TArray<FVector>& points, float tension);

	// La fonction mathématique pure de Catmull
	FVector CalculateCatmull(FVector P0, FVector P1, FVector P2, FVector P3, float T);

#if WITH_EDITORONLY_DATA
    UPROPERTY(Transient)
    ULineBatchComponent* EditorLineBatch = nullptr;
#endif
	
};
