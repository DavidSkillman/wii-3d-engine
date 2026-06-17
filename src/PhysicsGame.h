#pragma once
#ifndef PHYSICSGAME_H
#define PHYSICSGAME_H

#include <asndlib.h>
#include <WireApplication.h>
#include <btBulletDynamicsCommon.h>

#include "small-impact_01_raw.h"
#include "small-impact_02_raw.h"
#include "small-impact_03_raw.h"
#include "small-impact_04_raw.h"
#include "small-impact_05_raw.h"
#include "small-impact_06_raw.h"
#include "small-impact_07_raw.h"

#include "character_wmesh.h"
#include "box_wmesh.h"

const uint8_t* impact_sounds[] = { small_impact_01_raw, small_impact_02_raw, small_impact_03_raw, small_impact_04_raw, small_impact_05_raw, small_impact_06_raw, small_impact_07_raw };
const size_t impact_sounds_size[] = { small_impact_01_raw_size, small_impact_02_raw_size, small_impact_03_raw_size, small_impact_04_raw_size, small_impact_05_raw_size, small_impact_06_raw_size, small_impact_07_raw_size };

namespace Wire
{
	struct MeshMaterial {
		Float r, g, b;
		Float roughness;
		Float metallic;
	};

	class PhysicsGame : public WIREAPPLICATION
	{
		WIRE_DECLARE_INITIALIZE;

		typedef WIREAPPLICATION Parent;

	public:
		PhysicsGame();
		virtual Bool OnInitialize();
		virtual void OnIdle();
		virtual void OnTerminate();
		virtual void OnInput();

	private:
		struct GameObject  
		{
			GameObject() {};
			GameObject(RenderObject* pRenderObject, btCollisionShape* pCol,
				btRigidBody* pRigidBody)
			{
				spRenderObjects = WIRE_NEW TArray<RenderObjectPtr>();
				spRenderObjects->Append(pRenderObject);
				pCollisionObject = pCol;
				pRigidBody = pRigidBody;
			}

			GameObject(TArray<RenderObjectPtr>* pRenderObjects, btCollisionShape* pCol,
				btRigidBody* pRigidBody)
			{
				spRenderObjects = pRenderObjects;
				pCollisionObject = pCol;
				pRigidBody = pRigidBody;
			}

			TArray<RenderObjectPtr>* spRenderObjects;
			Transformation WorldTransformation;
			Float movement;
			btCollisionShape* pCollisionObject;
			btRigidBody* pRigidBody;
		};

		void CreateGameObjects();
		static TArray<RenderObjectPtr>* CreateModel(const UChar* ptr, bool dbg = false);
		static Material* CreateMaterial(Vector3F color);
		static Material* CreateCheckerBoardMaterial(Vector3F primaryColor, Vector3F secondaryColor);
		Vector3F camPos;
		Float simSpeed = 1.f;
		void CreatePhysicsWorld();
		btRigidBody* CreateRigidBody(btCollisionShape* pColShape, Float mass,
			Vector3F position, Float extent = 1.0F);
		void ResetRigidBodies();
		//Texture2D* CreateTexture();

		void RandomizeBallVelocity(btRigidBody* pBody);
		void UpdatePhysicsWorld(btScalar elapsedTime);
		void DestroyPhysicsWorld();
		Vector3F LocalToGlobal(Vector3F rotation, Vector3F forwardVector);
		void SetRotation(Vector3F rotation);
		Vector3F camRotation;
		RenderObject* pFloor;

		inline btVector3 Convert(Vector3F in)
		{
			return btVector3(in.X(), in.Y(), in.Z());
		}

		inline Vector3F Convert(btVector3& in)
		{
			return Vector3F(in.getX(), in.getY(), in.getZ());
		}

		inline QuaternionF Convert(const btQuaternion& in)
		{
			return QuaternionF(in.getW(), in.getX(), in.getY(), in.getZ());
		}

		static Vector3F TransformPoint(const Matrix4F& m, const Vector3F& v)
		{
			return Vector3F(
				m[0][0] * v.X(),
				m[1][1] * v.Y(),
				m[2][2] * v.Z()
			);
		}

		CameraPtr mspCamera;
		Culler mCuller;

		TArray<GameObject> mGameObjects;

		TArray<RenderObjectPtr>* mspBox;
		RenderObjectPtr mspBall;
		BoundingVolumePtr mspWorldBound;
		MaterialPtr mspMaterial;
		StateMaterialPtr mspStateMaterial;
		LightPtr mspLightA;
		LightPtr mspLightB;

		Vector3F camRotVel;
		Vector3F camPosVel;

		Float mAngle;
		Double mLastTime;
		Double elapsedTime;
		Random mRandom;

		btDefaultCollisionConfiguration* mpCollisionConfiguration;
		btCollisionDispatcher* mpDispatcher;
		btAxisSweep3* mpOverlappingPairCache;
		btSequentialImpulseConstraintSolver* mpSolver;
		btDiscreteDynamicsWorld* mpDynamicsWorld;

		static const UInt s_BoxCountX = 5;
		static const UInt s_BoxCountY = 6;
	};

	WIRE_REGISTER_INITIALIZE(PhysicsGame);

}

#endif
