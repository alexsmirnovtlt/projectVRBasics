// Alex Smirnov 2020-2021


#include "HandActor.h"

#include "Components/SkeletalMeshComponent.h"

#include "ActorComponents/HandCollisionUpdaterComponent.h"
#include "VRMotionControllerHand.h"


AHandActor::AHandActor()
{
	PrimaryActorTick.bCanEverTick = true;

	HandMass = 10.f;
	RootBoneName = TEXT("hand_r");
	PalmSocketName = TEXT("palm_socket");
	NoCollisionPresetName = TEXT("None");
	ActiveCollisionPresetName = TEXT("PhysicsActor");
	OverlapSpherePresetName = TEXT("None");

	SetTickGroup(ETickingGroup::TG_PrePhysics);

	HandCollisionUpdaterComponent = CreateDefaultSubobject<UHandCollisionUpdaterComponent>(TEXT("HandCollisionUpdaterComponent"));
}

void AHandActor::BeginPlay()
{
	Super::BeginPlay();

	auto HandMesh = GetSkeletalHandMeshComponent();
	if (!HandMesh) return;

	HandMesh->SetUseCCD(true);

	ChangeHandPhysProperties(false, false); // Before we setup our phys costraint hand should not collide with anything
	
	HandCollisionUpdaterComponent->SetupWeldedBoneDriver(HandMesh); // This component updates PhysicsAsset shapes with current bone locations every frame so animation changes affect hand collisions too

	// Setting Collision to hand sphere so it should only create overlap events only with components that need to interact with that sphere to get some sort of control over hand (grabbable object)
	if (auto CollisionSphere = GetCollisionSphereComponent())
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		CollisionSphere->SetGenerateOverlapEvents(true);

		if (OverlapSpherePresetName.IsNone())
		{
			CollisionSphere->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
			CollisionSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
		}
		else CollisionSphere->SetCollisionProfileName(OverlapSpherePresetName);
	}
}

void AHandActor::SetupHandSphereCollisionCallbacks(AVRMotionControllerHand* VRMotionController)
{
	if (auto CollisionSphere = GetCollisionSphereComponent())
	{
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(VRMotionController, &AVRMotionControllerHand::HandCollisionSphereBeginOverlap);
		CollisionSphere->OnComponentEndOverlap.AddDynamic(VRMotionController, &AVRMotionControllerHand::HandCollisionSphereEndOverlap);
	}
}

void AHandActor::RemoveHandSphereCollisionCallbacks(AVRMotionControllerHand* VRMotionController)
{
	if (auto CollisionSphere = GetCollisionSphereComponent())
	{
		CollisionSphere->OnComponentBeginOverlap.RemoveDynamic(VRMotionController, &AVRMotionControllerHand::HandCollisionSphereBeginOverlap);
		CollisionSphere->OnComponentEndOverlap.RemoveDynamic(VRMotionController, &AVRMotionControllerHand::HandCollisionSphereEndOverlap);
	}
}

void AHandActor::ChangeHandPhysProperties(bool bEnableCollision, bool bSimulatePhysics)
{
	auto HandMesh = GetSkeletalHandMeshComponent();
	if (!HandMesh) return;

	HandMesh->SetSimulatePhysics(bSimulatePhysics);

	if(bEnableCollision) HandMesh->SetCollisionProfileName(ActiveCollisionPresetName);
	else
	{
		if (NoCollisionPresetName.IsNone())
		{
			HandMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // No Collision Profile was specified, ignoring all channels but enabling query and phys collisions for physical constraint
			HandMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		}
		else HandMesh->SetCollisionProfileName(NoCollisionPresetName);
	}
}

void AHandActor::RefreshWeldedBoneDriver()
{
	HandCollisionUpdaterComponent->RefreshWeldedBoneDriver();
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

UPrimitiveComponent* AHandActor::GetCollisionSphereComponent_Implementation() const
{
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