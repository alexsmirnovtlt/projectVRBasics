// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "HandActor.generated.h"

UCLASS(Blueprintable, abstract)
class PROJECTVRBASICS_API AHandActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AHandActor();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

protected:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Override")
	class USkeletalMeshComponent* GetSkeletalHandMeshComponent() const;
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Override")
	class USceneComponent* GetArrowComponent() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Hand Setup") 
	float HandMass;


private:
	UPROPERTY(VisibleAnywhere, Category = "VR Hand Setup")
	class UVRHandPhysicalAnimationComponent* HandPhysicalAnimationComponent;
};
