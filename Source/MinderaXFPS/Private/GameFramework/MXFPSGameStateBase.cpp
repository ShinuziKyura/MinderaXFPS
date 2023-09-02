// Ricardo Santos, 2023

#include "GameFramework/MXFPSGameStateBase.h"

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
}

FMatrix AMXFPSGameStateBase::GetPlayerViewMatrix() const
{
	return CachedViewMatrices.GetViewProjectionMatrix();
}

FVector AMXFPSGameStateBase::GetPlayerViewOrigin() const
{
	return CachedViewMatrices.GetViewOrigin();
}

bool AMXFPSGameStateBase::IsActorWithinPlayerFrustum(AActor* Actor, TArray<FVector>& OutSupportVertexes) const
{
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

	FMatrix InvViewRotMat = CachedViewInitOptions.ViewRotationMatrix.InverseFast();
	FMatrix ViewProjMat = CachedViewMatrices.GetViewProjectionMatrix();
	FIntRect ConstrainedViewRect = CachedViewInitOptions.GetConstrainedViewRect();

	FVector WorldRightDir = InvViewRotMat.GetScaledAxis(EAxis::X);
	FVector WorldLeftDir = -WorldRightDir;
	FVector WorldUpDir = InvViewRotMat.GetScaledAxis(EAxis::Y);
	FVector WorldDownDir = -WorldUpDir;

	static FVector2D ScreenLeftDir(-1.f, 0.f);
	static FVector2D ScreenRightDir(+1.f, 0.f);
	static FVector2D ScreenUpDir(0.f, -1.f);
	static FVector2D ScreenDownDir(0.f, +1.f);
	
	FPlane RightPlane;
	ViewProjMat.GetFrustumRightPlane(RightPlane);
	
	FVector LeftSupportVertex = ComputeProjectedSupportVertex(ActorBoundsVertexes, ViewProjMat,
															  ConstrainedViewRect, ScreenLeftDir);
	float LeftSignedDistance = RightPlane.Flip().PlaneDot(LeftSupportVertex);
	
	if (LeftSignedDistance < 0)
	{
		return false;
	}

	FPlane LeftPlane;
	ViewProjMat.GetFrustumLeftPlane(LeftPlane);

	FVector RightSupportVertex = ComputeProjectedSupportVertex(ActorBoundsVertexes, ViewProjMat,
															   ConstrainedViewRect, ScreenRightDir);
	float RightSignedDistance = LeftPlane.Flip().PlaneDot(RightSupportVertex);
	
	if (RightSignedDistance < 0)
	{
		return false;
	}
	
	FPlane BottomPlane;
	ViewProjMat.GetFrustumBottomPlane(BottomPlane);
	
	FVector TopSupportVertex = ComputeProjectedSupportVertex(ActorBoundsVertexes, ViewProjMat,
															 ConstrainedViewRect, ScreenUpDir);
	float TopSignedDistance = BottomPlane.Flip().PlaneDot(TopSupportVertex);
	
	if (TopSignedDistance < 0)
	{
		return false;
	}
	
	FPlane TopPlane;
	ViewProjMat.GetFrustumTopPlane(TopPlane);
	
	FVector BottomSupportVertex = ComputeProjectedSupportVertex(ActorBoundsVertexes, ViewProjMat,
																ConstrainedViewRect, ScreenDownDir);
	float BottomSignedDistance = TopPlane.Flip().PlaneDot(BottomSupportVertex);
	
	if (BottomSignedDistance < 0)
	{
		return false;
	}

	OutSupportVertexes.Reserve(4);
	OutSupportVertexes.Add(LeftSupportVertex + WorldRightDir);
	OutSupportVertexes.Add(RightSupportVertex + WorldLeftDir);
	OutSupportVertexes.Add(TopSupportVertex + WorldDownDir);
	OutSupportVertexes.Add(BottomSupportVertex + WorldUpDir);

	return true;
}

void AMXFPSGameStateBase::BeginPlay()
{
	Super::BeginPlay();

	// Initialize ViewMatrices with something, just to avoid NAN warnings.
//	UpdateViewMatrices(GetWorld(), LEVELTICK_All, 0.f);
	
	FWorldDelegates::OnWorldPreActorTick.AddUObject(this, &AMXFPSGameStateBase::UpdateViewMatrices);
}

void AMXFPSGameStateBase::EndPlay(EEndPlayReason::Type const EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	FWorldDelegates::OnWorldPreActorTick.RemoveAll(this);
}

void AMXFPSGameStateBase::UpdateViewMatrices(UWorld* World, ELevelTick, float)
{
	check(World);

	QUICK_SCOPE_CYCLE_COUNTER(STAT_PlayerCharacter_UpdateViewMatrices);
	
	APlayerController* PlayerController = World->GetFirstPlayerController();
	ULocalPlayer* Player = PlayerController->GetLocalPlayer();

	Player->CalcSceneViewInitOptions(CachedViewInitOptions, Player->ViewportClient->Viewport, nullptr);

	CachedViewMatrices = FViewMatrices{ CachedViewInitOptions };
}
