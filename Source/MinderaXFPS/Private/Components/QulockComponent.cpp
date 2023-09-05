// Ricardo Santos, 2023

#include "Components/QulockComponent.h"

#include "Subsystem/PlayerViewDataCachingSubsystem.h"

DECLARE_STATS_GROUP(TEXT("Qulock Movement Logic"), STATGROUP_QulockMovement, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("Is Actor Within Player View"), STAT_IsActorWithinPlayerView, STATGROUP_QulockMovement);
DECLARE_CYCLE_STAT(TEXT("Is Actor Within Player Frustum"), STAT_IsActorWithinPlayerFrustum, STATGROUP_QulockMovement);

#define QULOCK_SHOULD_USE_PROJECTED_VERTEX_TRACES 1
#define QULOCK_SHOULD_SHOW_DEBUG_TRACES 0

// Theory: https://en.wikipedia.org/wiki/Supporting_hyperplane
// In summary, for each plane of the player's view frustum,
// we want to compare the first vertex of an actor (or its bounds)
// that would appear in the player's view if the actor were moving in the direction of the frustum,
// i.e., for the left plane, we want to find the vertex that, projected on the screen,
// would be the rightmost vertex of the actor.
// We'll do this for each plane, and if all points are beyond the respective planes,
// we'll test each point later through a raycast (from the view's origin).
// We need to find only a single raycast which hits to stop the actor from moving.

namespace
{
	template <class AllocatorType>
	FVector ComputeProjectedSupportVertex(TArray<FVector, AllocatorType> const& PolyVertexes,
										  FMatrix const& ViewProjMat, FIntRect const& ViewRect,
										  FVector2D const& Direction)
	{
		float MaxProjection = -TNumericLimits<float>::Max();

		FVector SupportVertex;
		for (FVector const& Vertex : PolyVertexes)
		{
			FVector2D ScreenVertex;
			FSceneView::ProjectWorldToScreen(Vertex, ViewRect, ViewProjMat, ScreenVertex);

			float Projection = Direction.Dot(ScreenVertex);
			if (Projection > MaxProjection)
			{
				MaxProjection = Projection;
				SupportVertex = Vertex;
			}
		}

		return SupportVertex;
	}

	FVector ProjectPointOntoPlane(FVector const& Point, FVector const& PlaneOrigin, FVector const& PlaneNormal)
	{
		return PlaneNormal * FVector::DotProduct(PlaneOrigin - Point, PlaneNormal) + Point;
	}
}

UQulockComponent::UQulockComponent(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UQulockComponent::BeginPlay()
{
	Super::BeginPlay();
	
	for (auto PlayerIndex : PlayersToCheck)
	{
		int32 PlayerControllerIndex = PlayerIndex - EAutoReceiveInput::Player0;
		if (ensureMsgf(PlayerControllerIndex != INDEX_NONE, TEXT("PlayersToCheck was improperly configured")))
		{
			auto PlayerControllerIter = GetWorld()->GetPlayerControllerIterator();
			PlayerControllerIter += PlayerControllerIndex;

			SetupPlayerToCheck(PlayerControllerIter->Get());
		}
	}
}

void UQulockComponent::SetupPlayerToCheck(APlayerController* PlayerController)
{
	bool bAlreadySetup;
	PlayerControllerSet.Add(PlayerController, &bAlreadySetup);
	
	if (!bAlreadySetup)
	{
		GetCachingSubsystem()->SetupPlayerViewDataUpdate(PlayerController);
	}
}

bool UQulockComponent::CanMoveThisFrame() const
{
	if (bCanOwnerMove.IsSet())
	{
		return bCanOwnerMove.GetValue();
	}

	bCanOwnerMove = true;
	for (auto Player : PlayerControllerSet)
	{
		AActor* TargetActor = GetOwner();
		if (auto Controller = Cast<AController>(TargetActor))
		{
			TargetActor = Controller->GetPawn();
		}
		if (IsActorWithinPlayerView(Player, TargetActor))
		{
			bCanOwnerMove = false;
			break;
		}
	}
	
	return bCanOwnerMove.GetValue();
}

// Having code in macros is not great for the maintainability of the code in the macros,
// but it's great for the method that uses the macros, which has far more important logic.
#if QULOCK_SHOULD_SHOW_DEBUG_TRACES
#	define QULOCK_DRAW_DEBUG_TRACE() \
		DrawDebugLine(World, ViewOrigin, Vertex,\
					  FColor::Yellow, false, 1.f, 255)
#	define QULOCK_DRAW_DEBUG_TRACE_RESULT() \
		DrawDebugPoint(World, HitResult.Location, 10.f,\
					   HitResult.GetActor() == Actor ? FColor::Green : FColor::Red, false, 1.f, 255)
#	define QULOCK_BREAK_TRACE()
#else
#	define QULOCK_DRAW_DEBUG_TRACE()
#	define QULOCK_DRAW_DEBUG_TRACE_RESULT()
#	define QULOCK_BREAK_TRACE() \
		break
#endif

bool UQulockComponent::IsActorWithinPlayerView(APlayerController* Player, AActor* Actor) const
{
	SCOPE_CYCLE_COUNTER(STAT_IsActorWithinPlayerView);
	
	auto World = GetWorld();
	auto PlayerViewData = GetCachingSubsystem()->GetPlayerViewData(Player);

	FVector const& ViewOrigin = PlayerViewData.ViewOrigin;
#if QULOCK_SHOULD_USE_PROJECTED_VERTEX_TRACES
	FMatrix const& ViewProjMatrix = PlayerViewData.ViewProjMatrix;

	TArray<FPlane, TFixedAllocator<4>> FrustumPlaneArray;
	FrustumPlaneArray.SetNum(4);

	ViewProjMatrix.GetFrustumLeftPlane(FrustumPlaneArray[0]);
	ViewProjMatrix.GetFrustumRightPlane(FrustumPlaneArray[1]);
	ViewProjMatrix.GetFrustumTopPlane(FrustumPlaneArray[2]);
	ViewProjMatrix.GetFrustumBottomPlane(FrustumPlaneArray[3]);
#endif

	bool bIsInView = false;

	TArray<FVector> SupportVertexArray;
	if (IsActorWithinPlayerFrustum(Player, Actor, SupportVertexArray))
	{
		bool bNewIsInView = true;
		for (FVector Vertex : SupportVertexArray)
		{
			// Extend trace a bit to ensure it hits the geometry;
			// We could make this into a UPROPERTY if necessary, but this is good enough for now.
			constexpr float TraceTolerance = 10.f;

			FVector TraceDirection = (Vertex - ViewOrigin).GetUnsafeNormal();
			Vertex += TraceDirection * TraceTolerance;

			QULOCK_DRAW_DEBUG_TRACE();
			
			FHitResult HitResult;
			if (World->LineTraceSingleByChannel(HitResult, ViewOrigin, Vertex, ECC_Visibility))
			{
				QULOCK_DRAW_DEBUG_TRACE_RESULT();

				// NOTE: this currently has a limitation: if the point is actually outside the view frustum,
				// it will still report as a hit, but the actor might not be visible anymore due to occluding actors.
				// QULOCK_SHOULD_USE_PROJECTED_VERTEX_TRACES tries to mitigate this,
				// and does it pretty decently, although not perfectly.
				
				bNewIsInView = HitResult.GetActor() == Actor;
#			if QULOCK_SHOULD_USE_PROJECTED_VERTEX_TRACES
				if (bNewIsInView)
				{
					bool bShouldProject = false;
				
					for (FPlane const& Plane : FrustumPlaneArray)
					{
						if (Plane.PlaneDot(Vertex) > 0)
						{
							auto PlaneNormal = Plane.GetNormal();
							Vertex = ProjectPointOntoPlane(Vertex, ViewOrigin, PlaneNormal);
							bShouldProject = true;
						}
					}
				
					if (bShouldProject)
					{
						QULOCK_DRAW_DEBUG_TRACE();
						
						HitResult.Reset();
						if (World->LineTraceSingleByChannel(HitResult, ViewOrigin, Vertex, ECC_Visibility))
						{
							QULOCK_DRAW_DEBUG_TRACE_RESULT();

							bNewIsInView = HitResult.GetActor() == Actor;
						}
					}
				
					if (bNewIsInView)
					{
						bIsInView = true;
						QULOCK_BREAK_TRACE();
					}
				}
#			else
				if (bNewIsInView)
				{
					bIsInView = true;
					QULOCK_BREAK_TRACE();
				}
#			endif
			}
		}
	}

	return bIsInView;
}

bool UQulockComponent::IsActorWithinPlayerFrustum(APlayerController* Player, AActor* Actor,
												  TArray<FVector>& OutSupportVertexes) const
{
	SCOPE_CYCLE_COUNTER(STAT_IsActorWithinPlayerFrustum);
	
	auto PlayerViewData = GetCachingSubsystem()->GetPlayerViewData(Player);
	
	FMatrix const& ViewProjMat = PlayerViewData.ViewProjMatrix;
	FMatrix const& InvViewRotMat = PlayerViewData.InvViewRotMatrix;
	FIntRect const& ViewRect = PlayerViewData.ViewRectangle;

	// TODO this is quite inaccurate... but it will work for now
	FBox ActorBounds = Actor->GetComponentsBoundingBox();

	FVector ActorBoundsPos = ActorBounds.GetCenter();
	FVector ActorBoundsSizeAbs = ActorBounds.GetExtent();
		
	TArray<FVector, TFixedAllocator<8>> ActorBoundsVertexes;
	ActorBoundsVertexes.Add(ActorBoundsPos + FVector(+ActorBoundsSizeAbs.X, +ActorBoundsSizeAbs.Y, +ActorBoundsSizeAbs.Z));
	ActorBoundsVertexes.Add(ActorBoundsPos + FVector(+ActorBoundsSizeAbs.X, +ActorBoundsSizeAbs.Y, -ActorBoundsSizeAbs.Z));
	ActorBoundsVertexes.Add(ActorBoundsPos + FVector(+ActorBoundsSizeAbs.X, -ActorBoundsSizeAbs.Y, +ActorBoundsSizeAbs.Z));
	ActorBoundsVertexes.Add(ActorBoundsPos + FVector(+ActorBoundsSizeAbs.X, -ActorBoundsSizeAbs.Y, -ActorBoundsSizeAbs.Z));
	ActorBoundsVertexes.Add(ActorBoundsPos + FVector(-ActorBoundsSizeAbs.X, +ActorBoundsSizeAbs.Y, +ActorBoundsSizeAbs.Z));
	ActorBoundsVertexes.Add(ActorBoundsPos + FVector(-ActorBoundsSizeAbs.X, +ActorBoundsSizeAbs.Y, -ActorBoundsSizeAbs.Z));
	ActorBoundsVertexes.Add(ActorBoundsPos + FVector(-ActorBoundsSizeAbs.X, -ActorBoundsSizeAbs.Y, +ActorBoundsSizeAbs.Z));
	ActorBoundsVertexes.Add(ActorBoundsPos + FVector(-ActorBoundsSizeAbs.X, -ActorBoundsSizeAbs.Y, -ActorBoundsSizeAbs.Z));

	static FVector2D ScreenLeftDir(-1.f, 0.f);
	static FVector2D ScreenRightDir(+1.f, 0.f);
	static FVector2D ScreenUpDir(0.f, -1.f);
	static FVector2D ScreenDownDir(0.f, +1.f);
	
	FPlane RightPlane;
	ViewProjMat.GetFrustumRightPlane(RightPlane);
	
	FVector LeftSupportVertex = ComputeProjectedSupportVertex(ActorBoundsVertexes, ViewProjMat,
															  ViewRect, ScreenLeftDir);

	float LeftSignedDistance = RightPlane.PlaneDot(LeftSupportVertex);
	if (LeftSignedDistance > 0)
	{
		return false;
	}

	FPlane LeftPlane;
	ViewProjMat.GetFrustumLeftPlane(LeftPlane);

	FVector RightSupportVertex = ComputeProjectedSupportVertex(ActorBoundsVertexes, ViewProjMat,
															   ViewRect, ScreenRightDir);

	float RightSignedDistance = LeftPlane.PlaneDot(RightSupportVertex);
	if (RightSignedDistance > 0)
	{
		return false;
	}
	
	FPlane BottomPlane;
	ViewProjMat.GetFrustumBottomPlane(BottomPlane);
	
	FVector TopSupportVertex = ComputeProjectedSupportVertex(ActorBoundsVertexes, ViewProjMat,
															 ViewRect, ScreenUpDir);

	float TopSignedDistance = BottomPlane.PlaneDot(TopSupportVertex);
	if (TopSignedDistance > 0)
	{
		return false;
	}
	
	FPlane TopPlane;
	ViewProjMat.GetFrustumTopPlane(TopPlane);
	
	FVector BottomSupportVertex = ComputeProjectedSupportVertex(ActorBoundsVertexes, ViewProjMat,
																ViewRect, ScreenDownDir);

	float BottomSignedDistance = TopPlane.PlaneDot(BottomSupportVertex);
	if (BottomSignedDistance > 0)
	{
		return false;
	}
	
	// These are the axes of the player view's frame of reference
	FVector WorldRightDir = InvViewRotMat.GetScaledAxis(EAxis::X);
	FVector WorldLeftDir = -WorldRightDir;
	FVector WorldUpDir = InvViewRotMat.GetScaledAxis(EAxis::Y);
	FVector WorldDownDir = -WorldUpDir;

	OutSupportVertexes.Reserve(4);
	
	// We'll push the vertexes into the mesh a bit to ensure the raycast always hits when it should
	OutSupportVertexes.Add(LeftSupportVertex + WorldRightDir);
	OutSupportVertexes.Add(RightSupportVertex + WorldLeftDir);
	OutSupportVertexes.Add(TopSupportVertex + WorldDownDir);
	OutSupportVertexes.Add(BottomSupportVertex + WorldUpDir);

	return true;
}

FMatrix UQulockComponent::GetPlayerViewProjMatrix(APlayerController* Player) const
{
	auto PlayerViewData = GetCachingSubsystem()->GetPlayerViewData(Player);
	return PlayerViewData.ViewProjMatrix;
}

FVector UQulockComponent::GetPlayerViewOrigin(APlayerController* Player) const
{
	auto PlayerViewData = GetCachingSubsystem()->GetPlayerViewData(Player);
	return PlayerViewData.ViewOrigin;
}

UPlayerViewDataCachingSubsystem* UQulockComponent::GetCachingSubsystem() const
{
	return CachingSubsystem
		 ? CachingSubsystem
		 : CachingSubsystem = GetWorld()->GetSubsystem<UPlayerViewDataCachingSubsystem>();
}
