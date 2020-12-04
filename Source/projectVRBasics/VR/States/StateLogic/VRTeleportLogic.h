// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"


#include "VRTeleportLogic.generated.h"

struct FNavLocation;
struct FStreamableHandle;
struct FPredictProjectilePathPointData;

class USplineComponent;
class UNavigationSystemV1;
class USplineMeshComponent;

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class PROJECTVRBASICS_API UVRTeleportLogic : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	void Initialize(USplineComponent* SplineComponentReference);
	UFUNCTION(BlueprintCallable)
	void HandleDestruction();

	UFUNCTION(BlueprintCallable)
	AActor* GetArrowActor();

	UFUNCTION(BlueprintCallable)
	void UpdateTeleportArc(float HorizontalInput, float VerticalInput, FVector StartLocation, FRotator StartRotation);
	UFUNCTION(BlueprintCallable)
	void HideTeleportArc(bool bHideArrow = true);
	UFUNCTION(BlueprintCallable)
	void PerformTeleport();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSoftClassPtr<AActor> TeleportArrowClass;
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSoftClassPtr<USplineMeshComponent> TeleportBeamPartClass;
	UPROPERTY(EditDefaultsOnly, Category="Setup")
	int32 InitialSplineMeshPoolSize = 20;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectilePathParams")
	float TeleportProjectileRadius = 3.f;
	UPROPERTY(EditDefaultsOnly, Category = "ProjectilePathParams")
	float TeleportProjectileSpeed = 1400.f;
	UPROPERTY(EditDefaultsOnly, Category = "ProjectilePathParams")
	float TeleportSimulationTime = 1.f;

	TSharedPtr<FStreamableHandle> TeleportArrowHandle;
	TSharedPtr<FStreamableHandle> TeleportBeamHandle;

	UClass* TeleportBeamMeshLoadedClass;

	UPROPERTY()
	AActor* TeleportArrowActor;

	UFUNCTION()
	void OnAssetLoaded();

	UPROPERTY(BlueprintReadonly)
	USplineComponent* SplineComponent;

	UPROPERTY()
	TArray<USplineMeshComponent*> SplineMeshPool;

private:

	UNavigationSystemV1* NavigationSystem;
	
	void DrawProjectilePath(TArray<FPredictProjectilePathPointData>& PointData);
	void UpdateTargetTeleportLocation(bool bHit, FNavLocation NavLocationStruct, float InputX, float InputY);

	void AddMeshToPool(int32 Count);
};
