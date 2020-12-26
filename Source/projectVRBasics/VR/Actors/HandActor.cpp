// Alex Smirnov 2020-2021


#include "HandActor.h"

#include "Components/SkeletalMeshComponent.h"

#include "ActorComponents/HandCollisionUpdaterComponent.h"


AHandActor::AHandActor()
{
	PrimaryActorTick.bCanEverTick = true;

	HandMass = 10.f;
	RootBoneName = TEXT("hand_r");
	PalmSocketName = TEXT("palm_socket");
	NoCollisionPresetName = TEXT("NoCollision");
	ActiveCollisionPresetName = TEXT("PhysicsActor");
	
	SetTickGroup(ETickingGroup::TG_PrePhysics);

	HandCollisionUpdaterComponent = CreateDefaultSubobject<UHandCollisionUpdaterComponent>(TEXT("HandCollisionUpdaterComponent"));
}

void AHandActor::BeginPlay()
{
	Super::BeginPlay();

	auto HandMesh = GetSkeletalHandMeshComponent();
	if (!HandMesh) return;

	HandMesh->SetUseCCD(true);
	HandMesh->SetSimulatePhysics(false);
	HandMesh->SetMassOverrideInKg(NAME_None, HandMass);

	HandCollisionUpdaterComponent->SetupWeldedBoneDriver(HandMesh); // This component updates PhysicsAsset shapes with current bone locations every frame so animation changes affect hand collisions too
}

USkeletalMeshComponent* AHandActor::GetSkeletalHandMeshComponent_Implementation() const
{
	UE_LOG(LogTemp, Error, TEXT("Blueprint \"%s\" must override function GetSkeletalHandMeshComponent()"), *this->GetClass()->GetFName().ToString());
	return nullptr;
}

USceneComponent* AHandActor::GetArrowComponent_Implementation() const
{
	UE_LOG(LogTemp, Error, TEXT("Blueprint \"%s\" must override function GetArrowComponent()"), *this->GetClass()->GetFName().ToString());
	return nullptr;
}

float AHandActor::GetHandMass() const
{
	return HandMass;
}

FName& AHandActor::GetRootBoneName()
{
	return RootBoneName;
}

FName& AHandActor::GetPalmSocketName()
{
	return PalmSocketName;
}

FName& AHandActor::GetActiveCollisionPresetName()
{
	return ActiveCollisionPresetName;
}

FName& AHandActor::GeNoCollisionPresetName()
{
	return NoCollisionPresetName;
}