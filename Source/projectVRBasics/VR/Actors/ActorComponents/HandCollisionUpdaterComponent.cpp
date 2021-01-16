// Alex Smirnov 2020-2021


#include "HandCollisionUpdaterComponent.h"

#include "Components/SkeletalMeshComponent.h"


UHandCollisionUpdaterComponent::UHandCollisionUpdaterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	bAutoSetPhysicsSleepSensitivity = true;
	SleepThresholdMultiplier = 0.0f;
}

void UHandCollisionUpdaterComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateWeldedBoneDriver();
}

void UHandCollisionUpdaterComponent::SetupWeldedBoneDriver(USkeletalMeshComponent* SkeletalMesh, bool bSkipInit)
{
	SkeletalHandMesh = SkeletalMesh;
	if(!bSkipInit) SetupWeldedBoneDriver_Implementation(false);
}

void UHandCollisionUpdaterComponent::RefreshWeldedBoneDriver()
{
	SetupWeldedBoneDriver_Implementation(true);
}

FTransform UHandCollisionUpdaterComponent::GetWorldSpaceRefBoneTransform(FReferenceSkeleton& RefSkel, int32 BoneIndex, int32 ParentBoneIndex)
{
	FTransform BoneTransform;

	if (BoneIndex > 0 && BoneIndex != ParentBoneIndex)
	{
		BoneTransform = RefSkel.GetRefBonePose()[BoneIndex];

		FMeshBoneInfo BoneInfo = RefSkel.GetRefBoneInfo()[BoneIndex];
		if (BoneInfo.ParentIndex != 0 && BoneInfo.ParentIndex != ParentBoneIndex)
		{
			BoneTransform *= GetWorldSpaceRefBoneTransform(RefSkel, BoneInfo.ParentIndex, ParentBoneIndex);
		}
	}

	return BoneTransform;
}

FTransform UHandCollisionUpdaterComponent::GetRefPoseBoneRelativeTransform(FName BoneName)
{
	FTransform BoneTransform;

	if (SkeletalHandMesh && !BoneName.IsNone())
	{
		FReferenceSkeleton RefSkel;
		RefSkel = SkeletalHandMesh->SkeletalMesh->RefSkeleton;

		BoneTransform = GetWorldSpaceRefBoneTransform(RefSkel, RefSkel.FindBoneIndex(BoneName), 0);
	}

	return BoneTransform;
}

void UHandCollisionUpdaterComponent::SetupWeldedBoneDriver_Implementation(bool bReInit)
{
	TArray<FWeldedBoneDriverData> OriginalData;
	if (bReInit) OriginalData = BoneDriverMap;

	BoneDriverMap.Empty();

	if (!SkeletalHandMesh || !SkeletalHandMesh->Bodies.Num()) return;

	UPhysicsAsset* PhysAsset = SkeletalHandMesh->GetPhysicsAsset();
	if (!PhysAsset || !SkeletalHandMesh->SkeletalMesh) return;

#if WITH_PHYSX

	FBodyInstance* ParentBody = SkeletalHandMesh->Bodies[0];

	// Build map of bodies that we want to control.
	FPhysicsActorHandle& ActorHandle = ParentBody->WeldParent ? ParentBody->WeldParent->GetPhysicsActorHandle() : ParentBody->GetPhysicsActorHandle();
	if (!ActorHandle.IsValid()) return;

	FPhysicsCommand::ExecuteWrite(ActorHandle, [&](FPhysicsActorHandle& Actor)
	{
		PhysicsInterfaceTypes::FInlineShapeArray Shapes;
		FPhysicsInterface::GetAllShapes_AssumedLocked(Actor, Shapes);

		for (FPhysicsShapeHandle& Shape : Shapes)
		{
			FKShapeElem* ShapeElem = FPhysxUserData::Get<FKShapeElem>(FPhysicsInterface::GetUserData(Shape));
			if (ShapeElem)
			{
				FName TargetBoneName = ShapeElem->GetName();
				int32 BoneIdx = SkeletalHandMesh->GetBoneIndex(TargetBoneName);

				if (BoneIdx == INDEX_NONE) continue;

				FWeldedBoneDriverData DriverData;
				DriverData.BoneName = TargetBoneName;
				DriverData.ShapeHandle = Shape;

				if (bReInit && OriginalData.Num() - 1 >= BoneDriverMap.Num())
				{
					DriverData.RelativeTransform = OriginalData[BoneDriverMap.Num()].RelativeTransform;
				}
				else
				{
					FTransform BoneTransform = FTransform::Identity;
					if (SkeletalHandMesh->GetBoneIndex(TargetBoneName) != INDEX_NONE)
						BoneTransform = GetRefPoseBoneRelativeTransform(TargetBoneName).Inverse();

					DriverData.RelativeTransform = FPhysicsInterface::GetLocalTransform(Shape) * BoneTransform;
				}

				BoneDriverMap.Add(DriverData);
			}
		}

		if (bAutoSetPhysicsSleepSensitivity && !ParentBody->WeldParent && BoneDriverMap.Num() > 0)
		{
			ParentBody->SleepFamily = ESleepFamily::Custom;
			ParentBody->CustomSleepThresholdMultiplier = SleepThresholdMultiplier;
			float SleepEnergyThresh = FPhysicsInterface::GetSleepEnergyThreshold_AssumesLocked(Actor);
			SleepEnergyThresh *= ParentBody->GetSleepThresholdMultiplier();
			FPhysicsInterface::SetSleepEnergyThreshold_AssumesLocked(Actor, SleepEnergyThresh);
		}
	});
#endif
}


void UHandCollisionUpdaterComponent::UpdateWeldedBoneDriver()
{
	if (BoneDriverMap.Num() == 0 || !SkeletalHandMesh || SkeletalHandMesh->Bodies.Num() == 0) return;

	UPhysicsAsset* PhysAsset = SkeletalHandMesh->GetPhysicsAsset();
	if (!PhysAsset || !SkeletalHandMesh->SkeletalMesh) return;

#if WITH_PHYSX

	FBodyInstance* ParentBody = SkeletalHandMesh->Bodies[0];

	if (!ParentBody->IsInstanceSimulatingPhysics() && !ParentBody->WeldParent) return;

	FPhysicsActorHandle& ActorHandle = ParentBody->WeldParent ? ParentBody->WeldParent->GetPhysicsActorHandle() : ParentBody->GetPhysicsActorHandle();

	if (!FPhysicsInterface::IsValid(ActorHandle)) return;

	FPhysicsCommand::ExecuteWrite(ActorHandle, [&](FPhysicsActorHandle& Actor)
	{
		PhysicsInterfaceTypes::FInlineShapeArray Shapes;
		FPhysicsInterface::GetAllShapes_AssumedLocked(Actor, Shapes);

		FTransform GlobalPose = FPhysicsInterface::GetGlobalPose_AssumesLocked(ActorHandle).Inverse();

		for (FPhysicsShapeHandle& Shape : Shapes)
		{
			if (FWeldedBoneDriverData* WeldedData = BoneDriverMap.FindByKey(Shape))
			{
				FTransform Trans = SkeletalHandMesh->GetSocketTransform(WeldedData->BoneName, ERelativeTransformSpace::RTS_World);

				// This fixes a bug with simulating inverse scaled meshes
				//Trans.SetScale3D(FVector(1.f) * Trans.GetScale3D().GetSignVector());
				FTransform GlobalTransform = WeldedData->RelativeTransform * Trans;
				FTransform RelativeTM = GlobalTransform * GlobalPose;

				if (!WeldedData->LastLocal.Equals(RelativeTM))
				{
					FPhysicsInterface::SetLocalTransform(Shape, RelativeTM);
					WeldedData->LastLocal = RelativeTM;
				}
			}
		}
	});
#endif
}