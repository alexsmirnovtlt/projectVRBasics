// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "HandActor.generated.h"

class UHandCollisionUpdaterComponent;
class USkeletalMeshComponent;
class USceneComponent;

UCLASS(Blueprintable, abstract)
class PROJECTVRBASICS_API AHandActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AHandActor();

protected:
	virtual void BeginPlay() override;

public:
	float GetHandMass() const;

	UFUNCTION(BlueprintCallable, Category = "VR Hand")
	void ChangeHandPhysProperties(bool bEnableCollision, bool bSimulatePhysics);
	UFUNCTION(BlueprintCallable, Category = "VR Hand")
	void RefreshWeldedBoneDriver();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Override")
	USkeletalMeshComponent* GetSkeletalHandMeshComponent() const;
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Override")
	USceneComponent* GetArrowComponent() const;

	UFUNCTION(BlueprintCallable, Category = "VR Hand")
	FName& GetRootBoneName();
	UFUNCTION(BlueprintCallable, Category = "VR Hand")
	FName& GetPalmSocketName();
	UFUNCTION(BlueprintCallable, Category = "VR Hand")
	FName& GetActiveCollisionPresetName();
	UFUNCTION(BlueprintCallable, Category = "VR Hand")
	FName& GeNoCollisionPresetName();

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Hand Setup") 
	float HandMass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Hand Setup")
	FName RootBoneName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Hand Setup")
	FName PalmSocketName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Hand Setup")
	FName ActiveCollisionPresetName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Hand Setup")
	FName NoCollisionPresetName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UHandCollisionUpdaterComponent* HandCollisionUpdaterComponent;
};
