// PhysicsGame - Bullet Physics Integration
// This sample demonstrates how to use the Bullet Physics engine in Wire.

#include "PhysicsGame.h"

using namespace Wire;

WIRE_APPLICATION(PhysicsGame);

//----------------------------------------------------------------------------
PhysicsGame::PhysicsGame()
	:
	mAngle(0.0F),
	mLastTime(0.0),
	mpCollisionConfiguration(NULL),
	mpDispatcher(NULL),
	mpOverlappingPairCache(NULL),
	mpSolver(NULL),
	mpDynamicsWorld(NULL)
{
}

//----------------------------------------------------------------------------
Bool PhysicsGame::OnInitialize()
{
	if (!Parent::OnInitialize())
	{
		return false;
	}

	SND_Init();
	SND_Pause(0);

	// create the bullet objects necessary to run a rigid body simulation
	CreatePhysicsWorld();

	// create the physics objects and the rendering objects representing them
	CreateGameObjects();

	mspCamera = WIRE_NEW Camera;
	mspCamera->SetFrustum(90.0F, GetWidthF()/GetHeightF(), 0.1F, 300.0F);

	mspLightA = WIRE_NEW Light;
	mspLightA->Direction = Vector3F(0.5F, -1.0F, 0.5F);
	mspLightA->Color = ColorRGB::WHITE;

	mspLightB = WIRE_NEW Light;
	mspLightB->Direction = Vector3F(-0.5F, 1.0F, -0.5F);
	mspLightB->Color = ColorRGB(0.6F, 0.6F, 0.6F);

	mspStateMaterial = WIRE_NEW StateMaterial;
	mspStateMaterial->Ambient = ColorRGBA::WHITE;

	mspWorldBound = WIRE_NEW SphereBV;
	return true;
}

//----------------------------------------------------------------------------
void PhysicsGame::OnIdle()
{
	Double time = System::GetTime();
	elapsedTime = time - mLastTime;
	mLastTime = time;
	camRotation += Vector3F(camRotVel.X(), camRotVel.Y(), 0.f) * elapsedTime;
	camRotVel /= 1.25f;
	if (camRotation.X() < -M_PI_2)
		camRotation = Vector3F(-M_PI_2, camRotation.Y(), camRotation.Z());
	if (camRotation.X() > M_PI_2 - 0.00001)
		camRotation = Vector3F(M_PI_2 - 0.00001, camRotation.Y(), camRotation.Z());
	SetRotation(camRotation);

	camPos += camPosVel * elapsedTime;
	mspCamera->SetLocation(camPos);
	camPosVel /= 1.15f;

	mCuller.SetCamera(mspCamera);
	UpdatePhysicsWorld(static_cast<btScalar>(elapsedTime*simSpeed));
	GetRenderer()->ClearBuffers();
	GetRenderer()->PreDraw(mspCamera);

	GetRenderer()->SetClearColor(ColorRGBA(0.65f, 0.65f, 0.65f, 1.f));

	GetRenderer()->SetState(mspStateMaterial);
	GetRenderer()->SetLight(mspLightA, 0);
	GetRenderer()->SetLight(mspLightB, 1);
	GetRenderer()->EnableLighting();

	for (UInt i = 0; i < mGameObjects.GetQuantity(); i++)
	{
		for (UShort j = 0; j < mGameObjects[i].spRenderObjects->GetQuantity(); j++)
		{
			RenderObject* pRenderObject = mGameObjects[i].spRenderObjects->operator[](j);
			WIRE_ASSERT(pRenderObject);

			Transformation& rWorld = mGameObjects[i].WorldTransformation;
			pRenderObject->GetMesh()->GetModelBound()->TransformBy(rWorld,
				mspWorldBound);

			if (mCuller.IsVisible(mspWorldBound))
			{
				GetRenderer()->Draw(pRenderObject, rWorld);
			}
		}
	}

	for (Int i = 0; i < MAX_SND_VOICES; i++)
	{
		SND_ChangePitchVoice(i, 44100 * simSpeed);
	}

	GetRenderer()->PostDraw();
	GetRenderer()->DisplayBackBuffer();
}

//----------------------------------------------------------------------------
void PhysicsGame::OnTerminate()
{
	DestroyPhysicsWorld();
}

//----------------------------------------------------------------------------
void PhysicsGame::CreateGameObjects()
{
	WIRE_ASSERT(mpDynamicsWorld); // Has the physics world been created?

	//Material* pMaterial = CreateCheckerBoardMaterial(Vector3F(255.f, 16.f, 16.f), Vector3F(228.f, 16.f, 16.f));
	Material* pCBMaterial = CreateCheckerBoardMaterial(Vector3F(16.f, 16.f, 16.f), Vector3F(248.f, 248.f, 248.f));
	
	// create a few basic rigid bodies and their rendering representation
	///Please don't make the box sizes larger then 1000: the collision
	// detection will be inaccurate.
	///See http://www.continuousphysics.com/Bullet/phpBB2/viewtopic.php?t=346

	// create the (static, i.e. mass = 0) ground floor
	const Float floorX = 10;
	const Float floorY = 0.25F;
	const Float floorZ = 10;
	const Float floorMass = 0;

	// using a smart pointer in local scope here because we only need the 
	// RenderObject from the StandardMesh returned Node so the rest of the
	// node gets deleted automatically when returning from this function.
	pFloor = StandardMesh::CreateCube24(0, 0, true, 0.5F);

	btCollisionShape* pColFloor = WIRE_NEW btBoxShape(btVector3(
		btScalar(floorX), btScalar(floorY), btScalar(floorZ)));
	btRigidBody* pRigidFloor = CreateRigidBody(pColFloor, floorMass,
		Vector3F(0, -4.0F, 0));

	GameObject groundFloor(pFloor, pColFloor, pRigidFloor);
	groundFloor.WorldTransformation.SetScale(Vector3F(floorX * 2, floorY,
		floorZ * 2));
	mGameObjects.Append(groundFloor);

	TArray<RenderObjectPtr>* mspCharacter = CreateModel(character_wmesh);
	btCollisionShape* pColCharacter = WIRE_NEW btBoxShape(btVector3(
		btScalar(1), btScalar(1), btScalar(1)));
	btRigidBody* pRigidCharacter = CreateRigidBody(pColCharacter, 1.0f,
		Vector3F(0.0f, -4.0f, 3.0f));

	GameObject character(mspCharacter, pColCharacter, pRigidCharacter);
	character.WorldTransformation.SetScale(Vector3F::ONE * 0.05f);
	mGameObjects.Append(character);

	// 1 RenderObject Box to reuse 
	const Float extent = 0.5f;
	//mspBox = StandardMesh::CreateIcosahedron(extent*1.3, 1, true);
	mspBox = CreateModel(box_wmesh, true);

	// create a stack of boxes
	Bool switchX = false;
	Float y = -3.25F;
	
	for (UInt yCount = 0; yCount < s_BoxCountY;	yCount++)
	{
		Float x = static_cast<Float>(s_BoxCountX)*-0.5F;
		for (UInt xCount = 0; xCount < s_BoxCountX; xCount++)
		{
			btCollisionShape* pColBox = WIRE_NEW btBoxShape(btVector3(extent,
				extent, extent));
			const Float mass = 0.1F;
			btRigidBody* pRigidBox = CreateRigidBody(pColBox, mass,
				Vector3F(switchX ? x : x+0.35F, y, 0));

			GameObject box(mspBox, pColBox, pRigidBox);
			mGameObjects.Append(box);
			x += 1.25F;
		}

		y += 1.0F;
		switchX = !switchX;
	}


	// 1 RenderObject Ball to reuse 
	const Float radius = 0.75F;
	mspBall = StandardMesh::CreateSphere(16, 16, radius, 1, 0, true);
	mspBall->SetMaterial(pCBMaterial);

	// create 2 balls that collide with the boxes and cause them to collapse
	Float sign = 1;
	for (UInt i = 0; i < 2; i++)
	{
		btCollisionShape* pColBall = WIRE_NEW btSphereShape(radius);
		const Float mass = 1.0F;
		btRigidBody* pRigidBall = CreateRigidBody(pColBall, mass,
			Vector3F(1.5F * sign, -0.5F, 10 * sign));
		RandomizeBallVelocity(pRigidBall);

		GameObject ball(mspBall, pColBall, pRigidBall);
		mGameObjects.Append(ball);

		sign = -sign;
	}
}

UChar ReadU8(const UChar*& p)
{
    UChar v = *p;
    p++;
    return v;
}

UShort ReadU16(const UChar*& p)
{
    UShort v =
        (UShort)p[0] |
        (UShort)p[1] << 8;

    p += 2;
    return v;
}

UInt ReadU32(const UChar*& p)
{
    UInt v =
        (UInt)p[0] |
        (UInt)p[1] << 8 |
        (UInt)p[2] << 16 |
        (UInt)p[3] << 24;

    p += 4;
    return v;
}

float ReadFloat(const UChar*& p)
{
    UInt i =
        (UInt)p[0] |
        (UInt)p[1] << 8 |
        (UInt)p[2] << 16 |
        (UInt)p[3] << 24;

    p += 4;

    float f;
    memcpy(&f, &i, sizeof(float));
    return f;
}

TArray<RenderObjectPtr>* PhysicsGame::CreateModel(const UChar* ptr, bool dbg)
{
	char magic[]{0xDE, 0xEA, 0x7F, 0xDE, 0xFE, 0xA7, 0xED};
	if (memcmp(ptr, magic, 7) != 0)
	{
		WIRE_ASSERT(false); // Invalid mesh format.
		return WIRE_NEW TArray<RenderObjectPtr>();
	}
	
	ptr += 7;

	UInt materialCount = ReadU32(ptr);
	TArray<MeshMaterial*> materials;
	for (UInt i = 0; i < materialCount; i++)
	{
		MeshMaterial* material = WIRE_NEW MeshMaterial {
			ReadFloat(ptr),
			ReadFloat(ptr),
			ReadFloat(ptr),
			ReadFloat(ptr),
			ReadFloat(ptr)
		};
		materials.Append(material);
	}

	UChar boneCount = ReadU8(ptr);
	Matrix4F boneMatrices[boneCount];

	for (UChar i = 0; i < boneCount; i++) {
		boneMatrices[i] = Matrix4F(
			ReadFloat(ptr),	ReadFloat(ptr),	ReadFloat(ptr),	ReadFloat(ptr),
			ReadFloat(ptr),	ReadFloat(ptr),	ReadFloat(ptr),	ReadFloat(ptr),
			ReadFloat(ptr),	ReadFloat(ptr),	ReadFloat(ptr),	ReadFloat(ptr),
			ReadFloat(ptr),	ReadFloat(ptr),	ReadFloat(ptr),	ReadFloat(ptr)
		);
	}

	UChar boneParents[boneCount];

	for (UChar i = 0; i < boneCount; i++) {
		boneParents[i] = ReadU8(ptr);
	}

	Matrix4F worldBones[boneCount];

	for (uint8_t i = 0; i < boneCount; i++)
	{
		worldBones[i] = Matrix4F::IDENTITY;
	}

	for (uint8_t i = 0; i < boneCount; i++)
	{
		if (boneParents[i] == 0xFF)
		{
			worldBones[i] = boneMatrices[i];
		}
		else
		{
			worldBones[i] =
				worldBones[boneParents[i]] * boneMatrices[i];
		}
	}

	UShort chunkCount = ReadU16(ptr);

	TArray<RenderObjectPtr>* pModels = WIRE_NEW TArray<RenderObjectPtr>();
	for (UShort c = 0; c < chunkCount; c++)
	{
		UInt materialIndex = ReadU32(ptr);
		UShort vertexCount = ReadU16(ptr);
		UShort indexCount  = ReadU16(ptr);
		MeshMaterial* material = materials[materialIndex];

		VertexAttributes attributes;
		attributes.SetPositionChannels(3);
		attributes.SetColorChannels(3);
		attributes.SetNormalChannels(3);

		VertexBuffer* pVBuffer = WIRE_NEW VertexBuffer(attributes, vertexCount);

		for (UShort i = 0; i < vertexCount; i++)
		{
			Vector3F vertex = Vector3F(ReadFloat(ptr), ReadFloat(ptr), ReadFloat(ptr));
			Vector3F normal = Vector3F(
				std::max(std::min((std::max(ReadFloat(ptr) + 1.0f, material->roughness) - 1.0f) * (material->metallic + 1.0f) * 4.0f, 1.0f), -1.0f),
				std::max(std::min((std::max(ReadFloat(ptr) + 1.0f, material->roughness) - 1.0f) * (material->metallic + 1.0f) * 4.0f, 1.0f), -1.0f),
				std::max(std::min((std::max(ReadFloat(ptr) + 1.0f, material->roughness) - 1.0f) * (material->metallic + 1.0f) * 4.0f, 1.0f), -1.0f)
			);

			const UChar boneIDs[4] =
			{
				ReadU8(ptr),
				ReadU8(ptr),
				ReadU8(ptr),
				ReadU8(ptr)
			};

			const Float weights[4] =
			{
				ReadFloat(ptr),
				ReadFloat(ptr),
				ReadFloat(ptr),
				ReadFloat(ptr)
			};

			// Vector4F v(vertex.X(), vertex.Y(), vertex.Z(), 1.0f);

			// Vector3F result(0, 0, 0);

			// for (UChar i = 0; i < 4; i++)
			// {
			// 	Float w = weights[i];
			// 	if (w <= 0.0f)
			// 		continue;

			// 	UChar boneID = boneIDs[i];

			// 	// SAFETY: prevent crash / garbage reads
			// 	if (boneID >= boneCount)
			// 		continue;

			// 	Vector4F t = worldBones[boneID] * v;

			// 	result += Vector3F(t.X(), t.Y(), t.Z()) * w;
			// }
			
			//vertex = result;

			pVBuffer->Position3(i) = vertex;
			pVBuffer->Color3(i) = ColorRGB(255, 255, 255);
			pVBuffer->Normal3(i) = normal;
		}

		IndexBuffer* pIBuffer = WIRE_NEW IndexBuffer(indexCount);

		for (UShort i = 0; i < indexCount; i++)
		{
			(*pIBuffer)[i] = ReadU16(ptr);
		}

		pModels->Append(WIRE_NEW RenderObject(pVBuffer, pIBuffer, CreateMaterial(Vector3F(material->r * 255.0f, material->g * 255.0f, material->b * 255.0f))));
	}

	return pModels;
}

//----------------------------------------------------------------------------
Material* PhysicsGame::CreateMaterial(Vector3F color)
{
	const UInt width = 64;
	const UInt height = 64;
	const Image2D::FormatMode format = Image2D::FM_RGB888;
	const UInt bpp = Image2D::GetBytesPerPixel(format);
	UChar* const pDst = WIRE_NEW UChar[width * height * bpp];

	for (UInt y = 0; y < height; y++)
	{
		for (UInt x = 0; x < width; x++)
		{
			pDst[(y*width + x)*bpp] = color.X();  // R
			pDst[(y*width + x)*bpp+1] = color.Y(); // G
			pDst[(y*width + x)*bpp+2] = color.Z(); // B
		}
	}

	Image2D* pImage = WIRE_NEW Image2D(format, width, height, pDst);
	Texture2D* pTexture = WIRE_NEW Texture2D(pImage);

	Material* pMaterial = WIRE_NEW Material;
	pMaterial->AddTexture(pTexture, Material::BM_MODULATE);

	return pMaterial;
}

//----------------------------------------------------------------------------
Material* PhysicsGame::CreateCheckerBoardMaterial(Vector3F primaryColor, Vector3F secondaryColor)
{
	const UInt width = 2;
	const UInt height = 2;
	const Image2D::FormatMode format = Image2D::FM_RGB888;
	const UInt bpp = Image2D::GetBytesPerPixel(format);
	UChar* const pDst = WIRE_NEW UChar[width * height * bpp];

	Bool checker = false;
	for (UInt y = 0; y < height; y++)
	{
		for (UInt x = 0; x < width; x++)
		{
			pDst[(y*width + x)*bpp] = checker ? primaryColor.X() : secondaryColor.X();  // R
			pDst[(y*width + x)*bpp+1] = checker ? primaryColor.Y() : secondaryColor.Y(); // G
			pDst[(y*width + x)*bpp+2] = checker ? primaryColor.Z() : secondaryColor.Z(); // B
			checker = !checker;
		}
		checker = !checker;
	}

	Image2D* pImage = WIRE_NEW Image2D(format, width, height, pDst);
	Texture2D* pTexture = WIRE_NEW Texture2D(pImage);
	pTexture->SetFilterType(pTexture->FT_NEAREST);

	Material* pMaterial = WIRE_NEW Material;
	pMaterial->AddTexture(pTexture, Material::BM_MODULATE);

	return pMaterial;
}

//----------------------------------------------------------------------------
Vector3F PhysicsGame::LocalToGlobal(Vector3F rotation, Vector3F forwardVector)
{
    QuaternionF targetRotation = QuaternionF().FromAxisAngle(Vector3F::UNIT_Y, rotation.Y()) *
                                 QuaternionF().FromAxisAngle(Vector3F::UNIT_X, rotation.X()) *
                                 QuaternionF().FromAxisAngle(Vector3F::UNIT_Z, rotation.Z());
	return targetRotation.Rotate(forwardVector);
}

//----------------------------------------------------------------------------
void PhysicsGame::SetRotation(Vector3F rotation)
{
    Vector3F location = mspCamera->GetLocation();
	Vector3F forwardVector(0.f, 0.f, 1.f);
	Vector3F targetPosition = location + LocalToGlobal(rotation, forwardVector);

    mspCamera->LookAt(location, targetPosition, Vector3F::UNIT_Y);
}


void PhysicsGame::OnInput()
{
	UInt playerCount = GetInputSystem()->GetMainDevicesCount();
	playerCount = playerCount > 1 ? 1 : playerCount;

	if (playerCount == 0)
	{
		return;
	}

	const MainInputDevice* pInputDevice = GetInputSystem()->GetMainDevice(0);
	const Buttons* pButtons = DynamicCast<const Buttons>(pInputDevice->
			GetCapability(Buttons::TYPE, false));
	const DigitalPad* pDPad = DynamicCast<const DigitalPad>(pInputDevice->
			GetCapability(DigitalPad::TYPE, false));

	if (!pButtons)
	{
		return;
	}
	if (!pDPad)
	{
		return;
	}
	
	if (pButtons->GetButton(Buttons::BUTTON_HOME))
	{
		Parent::Close();
		return;
	}

	if (pInputDevice->HasExtension(Nunchuk::TYPE)) {
		const InputDeviceExtension* pNunchuk = pInputDevice->GetExtension(Nunchuk::TYPE);
		const Buttons* pNButtons = DynamicCast<const Buttons>(pNunchuk->
				GetCapability(Buttons::TYPE));
		const AnalogPad* pNAPad = DynamicCast<const AnalogPad>(pNunchuk->
				GetCapability(AnalogPad::TYPE));

		if (pNButtons->GetButton(Buttons::BUTTON_C)) {
			camPosVel = Vector3F(0.f, 10.f, 0.f);
		}
		if (pNButtons->GetButton(Buttons::BUTTON_Z)) {
			camPosVel = Vector3F(0.f, -10.f, 0.f);
		}

		if (pDPad->GetUp()) {
			camPosVel = LocalToGlobal(camRotation, Vector3F(0.f, 0.f, 10.f));
		}
		if (pDPad->GetLeft()) {
			camPosVel = LocalToGlobal(camRotation, Vector3F(10.f, 0.f, 0.f));
		}
		if (pDPad->GetRight()) {
			camPosVel = LocalToGlobal(camRotation, Vector3F(-10.f, 0.f, 0.f));
		}
		if (pDPad->GetDown()) {
			camPosVel = LocalToGlobal(camRotation, Vector3F(0.f, 0.f, -10.f));
		}

		camRotVel = Vector3F((pNAPad->GetDown() - pNAPad->GetUp()) * 2, (pNAPad->GetLeft() - pNAPad->GetRight()) * 2, camRotVel.Z());
	}
	else {
		if (pDPad->GetUp()) {
			camRotVel = Vector3F(-2.f, camRotVel.Y(), camRotVel.Z());
		}
		if (pDPad->GetLeft()) {
			camRotVel = Vector3F(camRotVel.X(), 2.f, camRotVel.Z());
		}
		if (pDPad->GetRight()) {
			camRotVel = Vector3F(camRotVel.X(), -2.f, camRotVel.Z());
		}
		if (pDPad->GetDown()) {
			camRotVel = Vector3F(2.f, camRotVel.Y(), camRotVel.Z());
		}
		
		if (pButtons->GetButton(Buttons::BUTTON_A)) {
			camPosVel = LocalToGlobal(camRotation, Vector3F(0.f, 0.f, 10.f));
		}
		if (pButtons->GetButton(Buttons::BUTTON_B)) {
			camPosVel = LocalToGlobal(camRotation, Vector3F(0.f, 0.f, -10.f));
		}

		if (pButtons->GetButton(Buttons::BUTTON_PLUS)) {
			camPosVel = Vector3F(0.f, 10.f, 0.f);
		}
		if (pButtons->GetButton(Buttons::BUTTON_MINUS)) {
			camPosVel = Vector3F(0.f, -10.f, 0.f);
		}
	}
	if (pButtons->GetButton(Buttons::BUTTON_1)) {
		ResetRigidBodies();
	}
	simSpeed = pButtons->GetButton(Buttons::BUTTON_2) ? (simSpeed - 0.25f) / 1.3f + 0.25f : 1.f;
}

//----------------------------------------------------------------------------
void PhysicsGame::CreatePhysicsWorld()
{
	///collision configuration contains default setup for memory.
	// Advanced users can create their own configuration.
	mpCollisionConfiguration = WIRE_NEW btDefaultCollisionConfiguration();

	///use the default collision dispatcher. For parallel processing you
	// can use a different dispatcher (see Extras/BulletMultiThreaded)
	mpDispatcher = WIRE_NEW btCollisionDispatcher(mpCollisionConfiguration);

	///the maximum size of the collision world. Make sure objects stay
	// within these boundaries
	///Don't make the world AABB size too large, it will harm simulation
	// quality and performance
	btVector3 worldAabbMin(-100, -100, -100);
	btVector3 worldAabbMax(100, 100, 100);
	UShort maxProxies = 1024;
	mpOverlappingPairCache = WIRE_NEW btAxisSweep3(worldAabbMin, worldAabbMax,
		maxProxies);

	///the default constraint solver. For parallel processing you can use
	// a different solver (see Extras/BulletMultiThreaded)
	mpSolver = WIRE_NEW btSequentialImpulseConstraintSolver;

	mpDynamicsWorld = WIRE_NEW btDiscreteDynamicsWorld(mpDispatcher,
		mpOverlappingPairCache, mpSolver, mpCollisionConfiguration);

	mpDynamicsWorld->setGravity(btVector3(0, -9.80665, 0));
}

//----------------------------------------------------------------------------
btRigidBody* PhysicsGame::CreateRigidBody(btCollisionShape* pColShape, Float mass,
	Vector3F position, Float extent /*= 1.0F*/)
{
	pColShape->setMargin(0.01F);

	/// Create Dynamic Objects
	btTransform trafo;
	trafo.setIdentity();
	btVector3 origin = Convert(position);
	trafo.setOrigin(origin);

	//rigidbody is dynamic if and only if mass is non zero
	Bool isDynamic = (mass != 0.0F);
	btVector3 localInertia(0, 0, 0);
	if (isDynamic)
	{
		pColShape->calculateLocalInertia(mass, localInertia);
	}

	//using motionstate is recommended, it provides interpolation
	//capabilities, and only synchronizes 'active' objects
	btDefaultMotionState* pMotionState = WIRE_NEW btDefaultMotionState(
		trafo);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, pMotionState,
		pColShape, localInertia);
	btRigidBody* pBody = WIRE_NEW btRigidBody(rbInfo);

	// Only do CCD if  motion in one timestep (1/60) exceeds 
	pBody->setCcdMotionThreshold(extent*0.5F);

	//Experimental: better estimation of CCD Time of Impact:
	pBody->setCcdSweptSphereRadius(extent*0.2F);

	mpDynamicsWorld->addRigidBody(pBody);
	return pBody;
}

//----------------------------------------------------------------------------
void PhysicsGame::ResetRigidBodies()
{
	if (!mpDynamicsWorld)
	{
		return;
	}

	Int objectCount = mpDynamicsWorld->getNumCollisionObjects();

	///create a copy of the array, not a reference!
	btCollisionObjectArray copy = mpDynamicsWorld->getCollisionObjectArray();

	for (Int i = 0; i < objectCount; i++)
	{
		btCollisionObject* pColObj = copy[i];
		btRigidBody* pBody = btRigidBody::upcast(pColObj);
		if (!pBody)
		{
			continue;
		}

		if (pBody->getMotionState())
		{
			btDefaultMotionState* pState = static_cast<
				btDefaultMotionState*>(pBody->getMotionState());
			pState->m_graphicsWorldTrans = pState->m_startWorldTrans;
			pBody->setCenterOfMassTransform(pState->m_graphicsWorldTrans);
			pColObj->setInterpolationWorldTransform(pState->m_startWorldTrans);
			pColObj->forceActivationState(ACTIVE_TAG);
			pColObj->activate();
			pColObj->setDeactivationTime(-1);
		}

		//removed cached contact points (this is not necessary if all
		// objects have been removed from the dynamics world)
		btBroadphaseInterface* pPhase = mpDynamicsWorld->getBroadphase();
		if (pPhase->getOverlappingPairCache())
		{
			pPhase->getOverlappingPairCache()->cleanProxyFromPairs(
				pColObj->getBroadphaseHandle(), mpDynamicsWorld->getDispatcher());
		}

		if (pBody && !pBody->isStaticObject())
		{
			pBody->setLinearVelocity(btVector3(0,0,0));
			pBody->setAngularVelocity(btVector3(0,0,0));

			if (pColObj->getCollisionShape()->getShapeType() ==  
				SPHERE_SHAPE_PROXYTYPE)
			{
				RandomizeBallVelocity(pBody);
			}
		}
	}

	///reset some internal cached data in the broadphase
	mpDynamicsWorld->getBroadphase()->resetPool(mpDynamicsWorld->
		getDispatcher());
	mpDynamicsWorld->getConstraintSolver()->reset();
}

//----------------------------------------------------------------------------
void PhysicsGame::RandomizeBallVelocity(btRigidBody* pBody)
{
	Float velZ = mRandom.GetFloat() + 0.5F;
	Float velY = (mRandom.GetFloat() - 0.5F) * 20.0F;
	btTransform trans;
	pBody->getMotionState()->getWorldTransform(trans);
	if (trans.getOrigin().z() > 0)
	{
		pBody->setLinearVelocity(btVector3(0, velY, -60 * velZ));
	}
	else
	{
		pBody->setLinearVelocity(btVector3(0, velY, 60 * velZ));
	}
}

//----------------------------------------------------------------------------
void PhysicsGame::UpdatePhysicsWorld(btScalar elapsedTime)
{
	// fixed 1/60 timestep
	mpDynamicsWorld->stepSimulation(elapsedTime, 60);

	Matrix3F mat;
	const btCollisionObjectArray& objectArray = mpDynamicsWorld->
		getCollisionObjectArray();

	for (Int i = 0; i < mpDynamicsWorld->getNumCollisionObjects(); i++)
	{
		btRigidBody* pBody = btRigidBody::upcast(objectArray[i]);
		if (pBody && pBody->getMotionState())
		{
			btTransform trans;
			pBody->getMotionState()->getWorldTransform(trans);
			Vector3F pos = Convert(trans.getOrigin());
			QuaternionF quat = Convert(trans.getRotation());
			quat.ToRotationMatrix(mat);
			mGameObjects[i].WorldTransformation.SetTranslate(pos);
			mGameObjects[i].WorldTransformation.SetRotate(mat);
			mGameObjects[i].movement = pos.X() * pos.Y() * pos.Z();
		}
	}

	for (Int i = 0; i < mpDynamicsWorld->getDispatcher()->getNumManifolds(); i++) {
		btPersistentManifold* contactManifold =
			mpDynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);

		// const btRigidBody* bodyA =
		// 	static_cast<const btRigidBody*>(contactManifold->getBody0());
		// const btRigidBody* bodyB =
		// 	static_cast<const btRigidBody*>(contactManifold->getBody1());

		Int numContacts = contactManifold->getNumContacts();

		float volume = 0.0f;
		float pan = 0.0f;
		float maxImpulse = 0.0f;
		for (int j = 0; j < numContacts; j++) {
			btManifoldPoint& pt = contactManifold->getContactPoint(j);

			if (pt.getDistance() < 0.0f) {
				btVector3 point = pt.getPositionWorldOnA();
				Vector3F dif = Convert(point) - mspCamera->GetLocation();

				float dist = dif.Normalize();
				float t = std::min(dist / 30.0f, 1.0f);
				volume = 1.0 - (t * t);
				pan = std::max(-1.0f, std::min(1.0f, dif.Dot(mspCamera->GetRVector())));

				float impulse = pt.getAppliedImpulse();
				if (impulse > maxImpulse) {
					maxImpulse = impulse;
				}
			}
		}
		if (maxImpulse > 0.2f) {
			Int s = mRandom.Get() % 7;
			float factor = (maxImpulse - 0.2f) / 3.0f;
			SND_SetVoice(SND_GetFirstUnusedVoice(), VOICE_STEREO_16BIT_LE, 44100, 0, (void*)impact_sounds[s], impact_sounds_size[s], (1.0f - pan) * factor * volume * 127.0f, (1.0f + pan) * factor * volume * 127.0f, NULL);
		}
	}
}

//----------------------------------------------------------------------------
void PhysicsGame::DestroyPhysicsWorld()
{
	//cleanup in the reverse order of creation/initialization

	//remove the rigidbodies from the dynamics world and delete them
	for (Int i = mpDynamicsWorld->getNumCollisionObjects()-1; i >= 0; i--)
	{
		btCollisionObject* pObj = mpDynamicsWorld->
			getCollisionObjectArray()[i];
		btRigidBody* pBody = btRigidBody::upcast(pObj);
		if (pBody && pBody->getMotionState())
		{
			WIRE_DELETE pBody->getMotionState();
		}

		mpDynamicsWorld->removeCollisionObject(pObj);
		WIRE_DELETE pObj;
	}

	//delete collision shapes
	for (UInt i = 0; i < mGameObjects.GetQuantity(); i++)
	{
		btCollisionShape* pShape = mGameObjects[i].pCollisionObject;
		WIRE_DELETE pShape;
	}

	WIRE_DELETE mpDynamicsWorld;
	WIRE_DELETE mpSolver;
	WIRE_DELETE mpOverlappingPairCache;
	WIRE_DELETE mpDispatcher;
	WIRE_DELETE mpCollisionConfiguration;
}
