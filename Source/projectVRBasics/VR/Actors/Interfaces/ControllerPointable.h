// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ControllerPointable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UControllerPointable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implements ability to be interacted with from a distance by pointing Motion Controller to it
 */
class PROJECTVRBASICS_API IControllerPointable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IControllerPointable")
	void OnGetPointed(AVirtualRealityMotionController* MotionController, USceneComponent* CollidedComponent, UPARAM(ref)FVector& HitLocation);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IControllerPointable")
	void OnEndPointed(AVirtualRealityMotionController* MotionController);
};
