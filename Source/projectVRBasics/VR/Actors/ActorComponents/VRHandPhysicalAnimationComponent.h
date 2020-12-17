// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"

#include "VRHandPhysicalAnimationComponent.generated.h"

USTRUCT()
struct FWeldedBoneDriverData
{
	GENERATED_BODY()

public:
	FTransform RelativeTransform;
	FName BoneName;
	FPhysicsShapeHandle ShapeHandle;

	FTransform LastLocal;

	FWeldedBoneDriverData() :
		RelativeTransform(FTransform::Identity),
		BoneName(NAME_None) {}

	FORCEINLINE bool operator==(const FPhysicsShapeHandle& Other) const
	{
		return (ShapeHandle == Other);
	}
};

/**
 * This is a slightly rewritten version of 
 * https://github.com/mordentral/VRExpPluginExample/blob/4.25-Locked/Plugins/VRExpansionPlugin/VRExpansionPlugin/Source/VRExpansionPlugin/Public/Misc/VREPhysicalAnimationComponent.h
 */
UCLASS(meta = (BlueprintSpawnableComponent), ClassGroup = Physics)
class PROJECTVRBASICS_API UVRHandPhysicalAnimationComponent : public UPhysicalAnimationComponent
{
	GENERATED_BODY()

public:

	UVRHandPhysicalAnimationComponent();
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WeldedBoneDriver)
	bool bAutoSetPhysicsSleepSensitivity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WeldedBoneDriver)
	float SleepThresholdMultiplier;

	void UpdateWeldedBoneDriver();

	UFUNCTION(BlueprintCallable, Category = PhysicalAnimation)
	void SetupWeldedBoneDriver();
	UFUNCTION(BlueprintCallable, Category = PhysicalAnimation)
	void RefreshWeldedBoneDriver();

	void SetupWeldedBoneDriver_Implementation(bool bReInit = false);

	FTransform GetWorldSpaceRefBoneTransform(FReferenceSkeleton& RefSkel, int32 BoneIndex, int32 ParentBoneIndex);
	FTransform GetRefPoseBoneRelativeTransform(USkeletalMeshComponent* SkeleMesh, FName BoneName);

private:

	UPROPERTY()
	TArray<FWeldedBoneDriverData> BoneDriverMap;
};
