#ifndef _PHYSX_EVENTCALLBACK_H_
#define _PHYSX_EVENTCALLBACK_H_

using namespace physx;

class PhysXEventCallback : public PxSimulationEventCallback
{
public:

	PhysXEventCallback() = default;
	virtual ~PhysXEventCallback() = default;

public:

	virtual void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override;
	virtual void onWake(PxActor** actors, PxU32 count) override;
	virtual void onSleep(PxActor** actors, PxU32 count) override;
	virtual void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;
	virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) override;
	virtual void onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) override;

};

class PhysXFillterCallback : public PxSimulationFilterCallback
{
public:

	PhysXFillterCallback() = default;
	virtual ~PhysXFillterCallback() = default;

public:

	virtual	PxFilterFlags pairFound(PxU32 pairID,
		PxFilterObjectAttributes attributes0, PxFilterData filterData0, const PxActor* a0, const PxShape* s0,
		PxFilterObjectAttributes attributes1, PxFilterData filterData1, const PxActor* a1, const PxShape* s1,
		PxPairFlags& pairFlags) override;

	virtual	void pairLost(PxU32 pairID,
		PxFilterObjectAttributes attributes0,
		PxFilterData filterData0,
		PxFilterObjectAttributes attributes1,
		PxFilterData filterData1,
		bool objectRemoved) override;

	virtual	bool statusChange(PxU32& pairID, PxPairFlags& pairFlags, PxFilterFlags& filterFlags) override;
};

PxFilterFlags PhysXFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0,
	PxFilterObjectAttributes attributes1, PxFilterData filterData1,
	PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize);

#endif // !_PHYSX_EVENTCALLBACK_H_
