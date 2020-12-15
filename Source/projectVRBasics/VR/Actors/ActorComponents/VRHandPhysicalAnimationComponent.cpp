// Alex Smirnov 2020-2021


#include "VRHandPhysicalAnimationComponent.h"

UVRHandPhysicalAnimationComponent::UVRHandPhysicalAnimationComponent()
{
	bAutoSetPhysicsSleepSensitivity = true;
	SleepThresholdMultiplier = 0.0f;
}

void UVRHandPhysicalAnimationComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateWeldedBoneDriver();
}

void UVRHandPhysicalAnimationComponent::SetupWeldedBoneDriver()
{
	SetupWeldedBoneDriver_Implementation(false);
}

void UVRHandPhysicalAnimationComponent::RefreshWeldedBoneDriver()
{
	SetupWeldedBoneDriver_Implementation(true);
}

FTransform UVRHandPhysicalAnimationComponent::GetWorldSpaceRefBoneTransform(FReferenceSkeleton& RefSkel, int32 BoneIndex, int32 ParentBoneIndex)
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

FTransform UVRHandPhysicalAnimationComponent::GetRefPoseBoneRelativeTransform(USkeletalMeshComponent* SkeleMesh, FName BoneName)
{
	FTransform BoneTransform;

	if (SkeleMesh && !BoneName.IsNone())
	{
		FReferenceSkeleton RefSkel;
		RefSkel = SkeleMesh->SkeletalMesh->RefSkeleton;

		BoneTransform = GetWorldSpaceRefBoneTransform(RefSkel, RefSkel.FindBoneIndex(BoneName), 0);
	}

	return BoneTransform;
}

void UVRHandPhysicalAnimationComponent::SetupWeldedBoneDriver_Implementation(bool bReInit)
{
	TArray<FWeldedBoneDriverData> OriginalData;
	if (bReInit)
	{
		OriginalData = BoneDriverMap;
	}

	BoneDriverMap.Empty();

	USkeletalMeshComponent* SkeleMesh = GetSkeletalMesh();

	if (!SkeleMesh || !SkeleMesh->Bodies.Num())
		return;

	UPhysicsAsset* PhysAsset = SkeleMesh ? SkeleMesh->GetPhysicsAsset() : nullptr;
	if (!PhysAsset || !SkeleMesh->SkeletalMesh)
	{
		return;
	}

#if WITH_PHYSX

	FBodyInstance* ParentBody = SkeleMesh->Bodies[0];
	
	// Build map of bodies that we want to control.
	FPhysicsActorHandle& ActorHandle = ParentBody->WeldParent ? ParentBody->WeldParent->GetPhysicsActorHandle() : ParentBody->GetPhysicsActorHandle();

	if (ActorHandle.IsValid())
	{
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
					int32 BoneIdx = SkeleMesh->GetBoneIndex(TargetBoneName);

					if (BoneIdx != INDEX_NONE)
					{
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
							if (SkeleMesh->GetBoneIndex(TargetBoneName) != INDEX_NONE)
								BoneTransform = GetRefPoseBoneRelativeTransform(SkeleMesh, TargetBoneName).Inverse();

							DriverData.RelativeTransform = FPhysicsInterface::GetLocalTransform(Shape) * BoneTransform;
						}

						BoneDriverMap.Add(DriverData);
					}
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
	}
#endif
}


void UVRHandPhysicalAnimationComponent::UpdateWeldedBoneDriver()
{
	if (!BoneDriverMap.Num())
		return;

	USkeletalMeshComponent* SkeleMesh = GetSkeletalMesh();

	if (!SkeleMesh || !SkeleMesh->Bodies.Num())
		return;

	UPhysicsAsset* PhysAsset = SkeleMesh ? SkeleMesh->GetPhysicsAsset() : nullptr;
	if (!PhysAsset || !SkeleMesh->SkeletalMesh)
	{
		return;
	}

#if WITH_PHYSX

	FBodyInstance* ParentBody = SkeleMesh->Bodies[0];

	if (!ParentBody->IsInstanceSimulatingPhysics() && !ParentBody->WeldParent)
		return;

	FPhysicsActorHandle& ActorHandle = ParentBody->WeldParent ? ParentBody->WeldParent->GetPhysicsActorHandle() : ParentBody->GetPhysicsActorHandle();

	if (FPhysicsInterface::IsValid(ActorHandle))
	{

		bool bModifiedBody = false;
		FPhysicsCommand::ExecuteWrite(ActorHandle, [&](FPhysicsActorHandle& Actor)
		{
			PhysicsInterfaceTypes::FInlineShapeArray Shapes;
			FPhysicsInterface::GetAllShapes_AssumedLocked(Actor, Shapes);

			FTransform GlobalPose = FPhysicsInterface::GetGlobalPose_AssumesLocked(ActorHandle).Inverse();

			for (FPhysicsShapeHandle& Shape : Shapes)
			{
				if (FWeldedBoneDriverData* WeldedData = BoneDriverMap.FindByKey(Shape))
				{
					bModifiedBody = true;

					FTransform Trans = SkeleMesh->GetSocketTransform(WeldedData->BoneName, ERelativeTransformSpace::RTS_World);

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
	}
#endif
}