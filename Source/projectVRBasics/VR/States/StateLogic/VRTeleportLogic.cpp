// Alex Smirnov 2020-2021


#include "VRTeleportLogic.h"

#include "TimerManager.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "Engine/AssetManager.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "GameFramework/PlayerController.h"

#include "../../../VR/Actors/VirtualRealityMotionController.h"
#include "../../../VR/Actors/VirtualRealityPawn.h"


void UVRTeleportLogic::Initialize(AVirtualRealityMotionController* MotionController)
{
	if (!ensure(MotionController && GetWorld())) return;

	OwningMotionController = MotionController;

	SplineComponent = OwningMotionController->GetSplineComponent();
	NavigationSystem = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	if (!ensure(SplineComponent && NavigationSystem)) return;
	
	// Async loading needed resources
	auto AssetManager = UAssetManager::GetIfValid();
	if (AssetManager)
	{
		FStreamableManager& StreamableManager = AssetManager->GetStreamableManager();

		TeleportArrowHandle = StreamableManager.RequestAsyncLoad(TeleportArrowClass.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &UVRTeleportLogic::OnAssetLoaded));
		TeleportBeamHandle = StreamableManager.RequestAsyncLoad(TeleportBeamPartClass.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &UVRTeleportLogic::OnAssetLoaded));
	}
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
		TeleportCollisionChannel,
		SplineComponent->GetOwner()
	);
	ProjectilePathParams.bTraceComplex = TeleportTraceComplex;

	FPredictProjectilePathResult Result;
	bool bHit = UGameplayStatics::PredictProjectilePath(this, ProjectilePathParams, Result);

	if (!bHit)
	{
		if(TeleportArrowActor) TeleportArrowActor->SetActorHiddenInGame(true);
		return;
	}

	DrawProjectilePath(Result.PathData); // Drawing Arc regardless if we found actual teleport spot or not

	// Placing Arrow if navigation mesh hit check successfull
	FNavLocation NavLocationStruct;
	bool NavCheckResult = NavigationSystem->ProjectPointToNavigation(Result.HitResult.Location, NavLocationStruct, NavMeshCheckExtent);
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
	if (!TeleportArrowActor || TeleportArrowActor->IsHidden()) return;

	HideTeleportArc(true);

	auto VRPawn = OwningMotionController->GetVRPawn();
	if (VRPawn && VRPawn->GetController())
	{
		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle_CameraFade,
			this,
			&UVRTeleportLogic::OnFadeTimerEnd,
			CameraFadeDurationSec,
			false
		);

		auto PlayerController = Cast<APlayerController>(VRPawn->GetController());
		if (PlayerController)
		{
			PlayerController->PlayerCameraManager->StartCameraFade(0.f, 1.f, CameraFadeDurationSec, FLinearColor::Black, false, true);
		}
	}
}

void UVRTeleportLogic::OnFadeTimerEnd()
{
	auto VRPawn = OwningMotionController->GetVRPawn();
	if (VRPawn && VRPawn->GetController())
	{
		VRPawn->TeleportToLocation(TeleportArrowActor->GetActorLocation(), TeleportArrowActor->GetActorRotation());

		auto PlayerController = Cast<APlayerController>(VRPawn->GetController());
		if (PlayerController)
		{
			PlayerController->PlayerCameraManager->StartCameraFade(1.f, 0.f, CameraFadeDurationSec, FLinearColor::Black);
		}
	}
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

	if (!bHit)
	{
		TeleportArrowActor->SetActorHiddenInGame(true);
		return;
	}

	FHitResult OutHit;
	FVector EndLocation = NavLocationStruct.Location + FVector::DownVector * 100.f;

	bool bRayTraceHit = GetWorld()->LineTraceSingleByChannel(OutHit, NavLocationStruct.Location, EndLocation, TeleportCollisionChannel);
	if (bRayTraceHit)
	{
		TeleportArrowActor->SetActorLocation(OutHit.Location);

		// Rotating arrow relative to motion controller and joystick input
		if (OwningMotionController)
		{
			auto ControllerWorldYaw = OwningMotionController->GetControllerWorldOriginRotation().Yaw;
			
			auto InputLocalRotation = FVector(-InputX, InputY, 0.f).Rotation();
			auto InputWorldRotation = FRotator(0.f, InputLocalRotation.Yaw - 90.f + ControllerWorldYaw, 0.f);

			TeleportArrowActor->SetActorRotation(InputWorldRotation);
		}

		TeleportArrowActor->SetActorHiddenInGame(false);
	}
	else
	{
		TeleportArrowActor->SetActorHiddenInGame(true);
	}
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