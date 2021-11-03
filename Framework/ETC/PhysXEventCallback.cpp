#include "PhysXEventCallback.h"

#include "Component.h"

void PhysXEventCallback::onConstraintBreak(PxConstraintInfo * constraints, PxU32 count)
{
}

void PhysXEventCallback::onWake(PxActor ** actors, PxU32 count)
{
}

void PhysXEventCallback::onSleep(PxActor ** actors, PxU32 count)
{
}

void PhysXEventCallback::onContact(const PxContactPairHeader & pairHeader, const PxContactPair * pairs, PxU32 nbPairs)
{
	GameObject* x = ((GameObject*)pairHeader.actors[0]->userData);
	GameObject* y = ((GameObject*)pairHeader.actors[1]->userData);

	if (pairs->events.isSet(PxPairFlag::eNOTIFY_TOUCH_FOUND))
	{
		x->OnCollisionEnter(y);
		y->OnCollisionEnter(x);
	}
	else if (pairs->events.isSet(PxPairFlag::eNOTIFY_TOUCH_PERSISTS))
	{
		x->OnCollisionStay(y);
		y->OnCollisionStay(x);
	}
	else if (pairs->events.isSet(PxPairFlag::eNOTIFY_TOUCH_LOST))
	{
		x->OnCollisionExit(y);
		y->OnCollisionExit(x);
	}
}

void PhysXEventCallback::onTrigger(PxTriggerPair * pairs, PxU32 count)
{
	Component* x = ((Component*)pairs->triggerActor->userData);
	Component* y = ((Component*)pairs->otherActor->userData);

	if (pairs->status & PxPairFlag::eNOTIFY_TOUCH_FOUND)
	{
		x->OnTriggerEnter(y->GetOwner());
		y->OnTriggerEnter(x->GetOwner());
	}
	else if (pairs->status & PxPairFlag::eNOTIFY_TOUCH_LOST)
	{
		x->OnTriggerExit(y->GetOwner());
		y->OnTriggerExit(x->GetOwner());
	}
}

void PhysXEventCallback::onAdvance(const PxRigidBody * const * bodyBuffer, const PxTransform * poseBuffer, const PxU32 count)
{
}

PxFilterFlags PhysXFillterCallback::pairFound(PxU32 pairID, PxFilterObjectAttributes attributes0, PxFilterData filterData0, const PxActor * a0, const PxShape * s0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, const PxActor * a1, const PxShape * s1, PxPairFlags & pairFlags)
{
	pairFlags |= PxPairFlag::eNOTIFY_TOUCH_PERSISTS | PxPairFlag::eNOTIFY_TOUCH_LOST;

	return PxFilterFlag::eCALLBACK | PxFilterFlag::eNOTIFY;
}

void PhysXFillterCallback::pairLost(PxU32 pairID, PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, bool objectRemoved)
{
}

bool PhysXFillterCallback::statusChange(PxU32 & pairID, PxPairFlags & pairFlags, PxFilterFlags & filterFlags)
{
	return false;
}

PxFilterFlags PhysXFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags & pairFlags, const void * constantBlock, PxU32 constantBlockSize)
{
	if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
	{
		pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
	}
	else
	{
		pairFlags = PxPairFlag::eCONTACT_DEFAULT;
	}

	pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;

	return PxFilterFlag::eCALLBACK;
}
