// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HandInteractable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UHandInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implements functionality related to interaction with VR controller
 */
class PROJECTVRBASICS_API IHandInteractable
{
	GENERATED_BODY()

public:

	// When physical hand overlaps with something, it tries to cast to to this interface and executes OnHandEnter
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnHandEnter(AVRMotionControllerHand* HandMotionController, USceneComponent* CollidedComponent);
	// When physical hand leaves overlapped area
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnHandExit(AVRMotionControllerHand* HandMotionController, USceneComponent* CollidedComponent);
	
	// Same as OnHandEnter, but collider is hand`s grab sphere, not the hand itself 
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnCanBeGrabbedByHand_Start(AVRMotionControllerHand* HandMotionController, USceneComponent* CollidedComponent);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnCanBeGrabbedByHand_End(AVRMotionControllerHand* HandMotionController, USceneComponent* CollidedComponent);

	// When phantom hand (actual motion controller world location that ignores any physics collision) overlaps something
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnPhantomHandEnter(AVRMotionControllerHand* HandMotionController, USceneComponent* CollidedComponent);
	// When phantom hand leaves overlapped area
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnPhantomHandExit(AVRMotionControllerHand* HandMotionController, USceneComponent* CollidedComponent);

	// When player input indicates that he wants to grab something
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnGrab(AVRMotionControllerHand* HandMotionController);
	// When player input indicates that he wants to drop something he already grabbed
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnDrop(AVRMotionControllerHand* HandMotionController);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnFinishedAttachingToHand();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnHandTeleported(AVRMotionControllerHand* HandMotionController);

	// When player tries to grab but overlaps multiple IHandInteractable actors, the one with the lowest distance will be chosen
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IHandInteractable")
	float GetWorldSquaredDistanceToMotionController(const AVRMotionControllerHand* HandMotionController) const;
	float GetWorldSquaredDistanceToMotionController_Implementation(const AVRMotionControllerHand* HandMotionController) const { return 0.f; };

	// Default False - Object is held only when we press button and drops on release. True - First button press will grab and second will drop
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IHandInteractable")
	bool IsRequiresSecondButtonPressToDrop() const; 
	bool IsRequiresSecondButtonPressToDrop_Implementation() const { return false; };

	// Can be set manually to true for some transitions or when this Actor is not ready to be grabbed
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IHandInteractable")
	bool IsGrabDisabled() const;
	bool IsGrabDisabled_Implementation() const { return false; };
	// Can be set manually to true for some transitions or when this Actor is not ready to be grabbed
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IHandInteractable")
	bool IsDropDisabled() const;
	bool IsDropDisabled_Implementation() const { return false; };
};
