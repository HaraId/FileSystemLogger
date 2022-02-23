#pragma once

#include <ntddk.h>
#include <wdf.h>



template<class TLock> 
class AutoLock
{
public:
	AutoLock(TLock& locker) : mLock(locker) {
		mLock.Lock();
	}
	~AutoLock() {
		mLock.Unlock();
	}

private:
	TLock& mLock;
};


class LockPoint {
public:
	virtual void Init() = 0;
	virtual void Lock() = 0;

	virtual void Unlock() = 0;
};


class Mutex : public LockPoint {
public:
	void Init() override {
		KeInitializeMutex(&mMutex, 0);
	}

	void Lock() override {
		KeWaitForSingleObject(&mMutex, Executive, KernelMode, FALSE, nullptr);
	}

	void Unlock() override {
		KeReleaseMutex(&mMutex, false);
	}


private:
	KMUTEX mMutex;
};

class FastMutex : public LockPoint {
public:
	void Init() override {
		ExInitializeFastMutex(&mMutex);
	}

	void Lock() override {
		ExAcquireFastMutex(&mMutex);
	}

	bool TryLock() {
		return ExTryToAcquireFastMutex(&mMutex);
	}

	void Unlock() override {
		ExReleaseFastMutex(&mMutex);
	}


private:
	FAST_MUTEX mMutex;
};

class SpinLockDispathLevel : public LockPoint {
public:
	void Init() override {
		KeInitializeSpinLock(&mSpinLock);
	}

	void Lock() override {
		KeAcquireSpinLock(&mSpinLock, &mOldIrql);
	}

	void Unlock() override {
		KeReleaseSpinLock(&mSpinLock, mOldIrql);
	}


private:
	KSPIN_LOCK mSpinLock;
	KIRQL mOldIrql;
};


class EventObject {
public:
	virtual void Init(EVENT_TYPE type = NotificationEvent, BOOLEAN State = FALSE) {
		KeInitializeEvent(&mEvent, type, State);
	}
	
	virtual void Set(KPRIORITY Inc, BOOLEAN Wait) {
		KeSetEvent(&mEvent, Inc, Wait);
	}

	virtual void Clear() {
		KeClearEvent(&mEvent);
	}

	virtual void Wait() {
		KeWaitForSingleObject(&mEvent, Executive, KernelMode, FALSE, NULL);
	}
	
protected:
	KEVENT mEvent;
};

