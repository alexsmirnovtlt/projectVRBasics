// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HandCollisionUpdaterComponent.generated.h"

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

class USkeletalMeshComponent;

/**
 * This is a slightly rewritten version of
 * https://github.com/mordentral/VRExpPluginExample/blob/4.25-Locked/Plugins/VRExpansionPlugin/VRExpansionPlugin/Source/VRExpansionPlugin/Public/Misc/VREPhysicalAnimationComponent.h
 */
UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup = Physics)
class PROJECTVRBASICS_API UHandCollisionUpdaterComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UHandCollisionUpdaterComponent();

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = PhysicalAnimation)
	void SetupWeldedBoneDriver(USkeletalMeshComponent* SkeletalMesh, bool bSkipInit = false);
	UFUNCTION(BlueprintCallable, Category = PhysicalAnimation)
	void RefreshWeldedBoneDriver();

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WeldedBoneDriver)
	bool bAutoSetPhysicsSleepSensitivity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WeldedBoneDriver)
	float SleepThresholdMultiplier;

	void UpdateWeldedBoneDriver();
	void SetupWeldedBoneDriver_Implementation(bool bReInit = false);

	FTransform GetWorldSpaceRefBoneTransform(FReferenceSkeleton& RefSkel, int32 BoneIndex, int32 ParentBoneIndex);
	FTransform GetRefPoseBoneRelativeTransform(FName BoneName);

private:

	UPROPERTY()
	USkeletalMeshComponent* SkeletalHandMesh;

	UPROPERTY()
	TArray<FWeldedBoneDriverData> BoneDriverMap;
};
