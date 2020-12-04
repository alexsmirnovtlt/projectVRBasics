// Alex Smirnov 2020-2021


#include "VRTeleportLogic.h"

#include "Engine/World.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StreamableManager.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"

#include "../../../General/GameInstances/GameInstWithStreamableManager.h"


void UVRTeleportLogic::Initialize(USplineComponent* SplineComponentReference)
{
	auto GameInstance = UGameplayStatics::GetGameInstance(this);
	if (!GameInstance || !SplineComponentReference || !GetWorld())
	{
		return;
	}

	SplineComponent = SplineComponentReference;

	auto GameInstanceWithStreamableManager = Cast<UGameInstWithStreamableManager>(GameInstance);
	if (!GameInstanceWithStreamableManager)
	{
		return;
	}
	
	// Async loading needed resources
	FStreamableManager& StreamableManager = GameInstanceWithStreamableManager->GetStreamableManager();
	
	TeleportArrowHandle = StreamableManager.RequestAsyncLoad(TeleportArrowClass.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &UVRTeleportLogic::OnAssetLoaded));
	TeleportBeamHandle = StreamableManager.RequestAsyncLoad(TeleportBeamPartClass.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &UVRTeleportLogic::OnAssetLoaded));
	//
	
	NavigationSystem = UNavigationSystemV1::GetNavigationSystem(GetWorld());
}

void UVRTeleportLogic::UpdateTeleportArc(float HorizontalInput, float VerticalInput, FVector StartLocation, FRotator StartRotation)
{
	if (!SplineComponent || !NavigationSystem) return;

	SplineComponent->ClearSplinePoints();
	HideTeleportArc(false);

	FPredictProjectilePathParams ProjectilePathParams(
		TeleportProjectileRadius,
		StartLocation,
		StartRotation.Vector().GetSafeNormal() * TeleportProjectileSpeed,
		TeleportSimulationTime,
		ECollisionChannel::ECC_WorldStatic,
		SplineComponent->GetOwner()
	);
	ProjectilePathParams.bTraceComplex = true;

	FPredictProjectilePathResult Result;
	bool bHit = UGameplayStatics::PredictProjectilePath(this, ProjectilePathParams, Result);

	if (!bHit)
	{
		if(TeleportArrowActor) TeleportArrowActor->SetActorHiddenInGame(true);
		return;
	}

	DrawProjectilePath(Result.PathData);

	// Placing Arrow if navigation mesh hit check successfull
	
	FNavLocation NavLocationStruct; // TODO change FVector(100, 100, 100));
	bool NavCheckResult = NavigationSystem->ProjectPointToNavigation(Result.HitResult.Location, NavLocationStruct, FVector(100, 100, 100));
	UpdateTargetTeleportLocation(NavCheckResult, NavLocationStruct, HorizontalInput, VerticalInput);
}

void UVRTeleportLogic::HideTeleportArc(bool bHideArrow)
{
	for (auto& MeshItem : SplineMeshPool)
	{
		MeshItem->SetVisibility(false);
	}

	if (bHideArrow && TeleportArrowActor)
	{
		TeleportArrowActor->SetActorHiddenInGame(true);
	}
}

void UVRTeleportLogic::PerformTeleport()
{
	if (TeleportArrowActor || TeleportArrowActor->IsHidden()) return;

	// TODO Teleport
}

void UVRTeleportLogic::DrawProjectilePath(TArray<FPredictProjectilePathPointData>& PointData)
{
	if (!SplineComponent) return;

	int32 ArraySizeDifference = PointData.Num() - SplineMeshPool.Num() - 1;
	if (ArraySizeDifference > 0) AddMeshToPool(ArraySizeDifference); // Need to create more meshes if PointData size is larger than pool`s size

	for (int32 i = 0; i < PointData.Num(); ++i)
	{
		FSplinePoint Point(i, PointData[i].Location, ESplinePointType::Curve);
		SplineComponent->AddSplinePoint(PointData[i].Location, ESplineCoordinateSpace::World, false);
	}

	SplineComponent->UpdateSpline();

	// Spline was updated, now placing meshes along it

	if (SplineMeshPool.Num() < PointData.Num() - 1) return; // Spline meshes were not yet created, doing nothing

	for (int32 i = 0; i < PointData.Num() - 1; ++i)
	{
		FVector StartPos, StartTangent, EndPos, EndTangent;
		SplineComponent->GetLocalLocationAndTangentAtSplinePoint(i, StartPos, StartTangent);
		SplineComponent->GetLocalLocationAndTangentAtSplinePoint(i + 1, EndPos, EndTangent);

		SplineMeshPool[i]->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
		SplineMeshPool[i]->SetVisibility(true);
	}
}

void UVRTeleportLogic::UpdateTargetTeleportLocation(bool bHit, FNavLocation NavLocationStruct, float InputX, float InputY)
{
	if (!TeleportArrowActor) return;

	TeleportArrowActor->SetActorHiddenInGame(!bHit);
	
	if (!bHit) return;

	// TODO project to static mesh and update rotation

	TeleportArrowActor->SetActorLocation(NavLocationStruct.Location);
}

void UVRTeleportLogic::OnAssetLoaded()
{
	if (!TeleportArrowHandle.IsValid() || !TeleportBeamHandle.IsValid())
	{
		return;
	}

	if (TeleportArrowHandle.Get()->HasLoadCompleted() && !TeleportArrowActor)
	{
		auto TeleportArrowLoadedClass = Cast<UClass>(TeleportArrowHandle.Get()->GetLoadedAsset());
		TeleportArrowActor = GetWorld()->SpawnActor<AActor>(TeleportArrowLoadedClass);
	}

	if (TeleportBeamHandle.Get()->HasLoadCompleted() && !TeleportBeamMeshLoadedClass)
	{
		TeleportBeamMeshLoadedClass = Cast<UClass>(TeleportBeamHandle.Get()->GetLoadedAsset());
		AddMeshToPool(InitialSplineMeshPoolSize);
	}
}

void UVRTeleportLogic::HandleDestruction()
{
	// TODO Check if thats enough

	if (TeleportArrowHandle.IsValid())
	{
		TeleportArrowHandle.Get()->ReleaseHandle();
	}
	if (TeleportBeamHandle.IsValid())
	{
		TeleportBeamHandle.Get()->ReleaseHandle();
	}

	if (TeleportArrowActor)
	{
		TeleportArrowActor->Destroy();
	}
	for (auto& SplineMesh : SplineMeshPool)
	{
		SplineMesh->DestroyComponent();
	}
}

void UVRTeleportLogic::AddMeshToPool(int32 NumberToAdd)
{
	if (!TeleportBeamMeshLoadedClass || !SplineComponent)
	{
		return;
	}

	for (uint8 i = 0; i < NumberToAdd; ++i)
	{
		USplineMeshComponent* NewSplineMesh = NewObject<USplineMeshComponent>(this, TeleportBeamMeshLoadedClass);
		NewSplineMesh->SetMobility(EComponentMobility::Movable);
		NewSplineMesh->AttachToComponent(SplineComponent, FAttachmentTransformRules::KeepRelativeTransform);
		NewSplineMesh->RegisterComponent();

		SplineMeshPool.Add(NewSplineMesh);
	}
}

AActor* UVRTeleportLogic::GetArrowActor()
{
	return TeleportArrowActor;
}