#include "ZFAnimation.h"

ZF_NAMESPACE_GLOBAL_BEGIN

ZFEXPORT_VAR_DEFINE(zftimet, ZFAnimationDurationDefault, 250)

// ============================================================
zfclass _ZFP_I_ZFAnimationAniList : zfextends ZFObject
{
    ZFOBJECT_DECLARE(_ZFP_I_ZFAnimationAniList, ZFObject)
public:
    ZFCoreArrayPOD<ZFAnimation *> aniList;
};

// ============================================================
// _ZFP_ZFAnimationPrivate
zfclassNotPOD _ZFP_ZFAnimationPrivate
{
public:
    ZFObject *aniTarget;
    zfbool aniRunning;
    zfbool aniDelaying;
    zfidentity aniDelayTaskId;
    zfidentity aniDelayThreadId;
    zfidentity aniDummyThreadId;
    zfbool aniStopCalled;
    zfbool aniStoppedByUser;
    zfidentity aniId;

public:
    _ZFP_ZFAnimationPrivate(void)
    : aniTarget(zfnull)
    , aniRunning(zffalse)
    , aniDelaying(zffalse)
    , aniDelayTaskId(zfidentityInvalid())
    , aniDelayThreadId(zfidentityInvalid())
    , aniDummyThreadId(zfidentityInvalid())
    , aniStopCalled(zffalse)
    , aniStoppedByUser(zffalse)
    , aniId(zfidentityInvalid())
    {
    }
};

// ============================================================
ZF_GLOBAL_INITIALIZER_INIT_WITH_LEVEL(ZFAnimationTaskHolder, ZFLevelZFFrameworkEssential)
{
    this->delayOnFinishListener = ZFCallbackForFunc(zfself::delayOnFinish);
    this->dummyOnFinishListener = ZFCallbackForFunc(zfself::dummyOnFinish);
}
public:
    ZFListener delayOnFinishListener;
    static ZFLISTENER_PROTOTYPE_EXPAND(delayOnFinish)
    {
        ZFAnimation *ani = userData->objectHolded();
        v_zfidentity *taskId = listenerData.param0<v_zfidentity *>();
        ani->_ZFP_ZFAnimation_aniImplDelayNotifyFinish(taskId->zfv);
    }
    ZFListener dummyOnFinishListener;
    static ZFLISTENER_PROTOTYPE_EXPAND(dummyOnFinish)
    {
        ZFAnimation *ani = userData->objectHolded();
        ani->_ZFP_ZFAnimation_aniDummyNotifyStop();
    }
ZF_GLOBAL_INITIALIZER_END(ZFAnimationTaskHolder)

// ============================================================
ZFOBJECT_REGISTER(ZFAnimation)

ZFOBSERVER_EVENT_REGISTER(ZFAnimation, AniOnInvalid)
ZFOBSERVER_EVENT_REGISTER(ZFAnimation, AniOnDelayFinish)
ZFOBSERVER_EVENT_REGISTER(ZFAnimation, AniOnStart)
ZFOBSERVER_EVENT_REGISTER(ZFAnimation, AniOnStop)
ZFOBSERVER_EVENT_REGISTER(ZFAnimation, AniOnStopOrOnInvalid)

void ZFAnimation::objectOnInit(void)
{
    zfsuper::objectOnInit();
    d = zfpoolNew(_ZFP_ZFAnimationPrivate);
}
void ZFAnimation::objectOnDealloc(void)
{
    zfpoolDelete(d);
    d = zfnull;
    zfsuper::objectOnDealloc();
}
void ZFAnimation::objectOnDeallocPrepare(void)
{
    this->aniStop();
    zfsuper::objectOnDeallocPrepare();
}

ZFMETHOD_DEFINE_1(ZFAnimation, void, aniTarget,
                  ZFMP_IN(ZFObject *, aniTarget))
{
    zfCoreAssertWithMessage(!d->aniRunning, "change animation's target while animation is running");
    d->aniTarget = aniTarget;
}
ZFMETHOD_DEFINE_0(ZFAnimation, ZFObject *, aniTarget)
{
    return d->aniTarget;
}

ZFMETHOD_DEFINE_0(ZFAnimation, void, aniStart)
{
    this->_ZFP_ZFAnimation_aniReadyStart();

    d->aniStoppedByUser = zffalse;
    if(!this->aniValid())
    {
        this->aniOnInvalid();
        this->aniOnStopOrOnInvalid(zffalse);
        return ;
    }

    zfRetain(this);
    zfRetain(this->aniTarget());

    d->aniRunning = zftrue;
    d->aniStopCalled = zffalse;
    ++(d->aniId);
    this->aniOnStart();

    if(this->aniDelay() > 0)
    {
        d->aniDelaying = zftrue;
        this->aniImplDelay();
    }
    else
    {
        d->aniDelaying = zffalse;
        this->aniImplStart();
    }
}
ZFMETHOD_DEFINE_0(ZFAnimation, zfbool, aniRunning)
{
    return d->aniRunning;
}
ZFMETHOD_DEFINE_0(ZFAnimation, zfbool, aniDelaying)
{
    return d->aniDelaying;
}
ZFMETHOD_DEFINE_0(ZFAnimation, void, aniStop)
{
    if(!(d->aniRunning) || d->aniStopCalled)
    {
        return ;
    }
    d->aniStopCalled = zftrue;
    d->aniStoppedByUser = zftrue;
    ++(d->aniId);
    if(d->aniDelaying)
    {
        d->aniDelaying = zffalse;
        this->aniImplDelayCancel();
    }
    else
    {
        this->aniImplStop();
    }
    this->aniImplNotifyStop();
}
ZFMETHOD_DEFINE_0(ZFAnimation, zfbool, aniStoppedByUser)
{
    return d->aniStoppedByUser;
}

ZFMETHOD_DEFINE_0(ZFAnimation, zfidentity, aniId)
{
    return d->aniId;
}

ZFMETHOD_DEFINE_0(ZFAnimation, zfbool, aniValid)
{
    return this->aniImplCheckValid();
}
zfbool ZFAnimation::aniImplCheckValid(void)
{
    return (this->aniDurationFixed() > 0);
}

void ZFAnimation::_ZFP_ZFAnimation_aniImplDelayNotifyFinish(ZF_IN zfidentity taskId)
{
    if(taskId == d->aniDelayTaskId)
    {
        this->aniImplDelayNotifyFinish();
    }
}
void ZFAnimation::_ZFP_ZFAnimation_aniReadyStart(void)
{
    if(this->aniTarget() != zfnull)
    {
        _ZFP_I_ZFAnimationAniList *aniList = this->aniTarget()->objectTag<_ZFP_I_ZFAnimationAniList *>(_ZFP_I_ZFAnimationAniList::ClassData()->classNameFull());
        if(aniList == zfnull)
        {
            aniList = zfAlloc(_ZFP_I_ZFAnimationAniList);
            this->aniTarget()->objectTag(_ZFP_I_ZFAnimationAniList::ClassData()->classNameFull(), aniList);
            zfRelease(aniList);
        }
        if(this->aniAutoStopPrev())
        {
            while(!aniList->aniList.isEmpty())
            {
                aniList->aniList.getFirst()->aniStop();
            }
        }
        aniList->aniList.add(this);
    }
}
void ZFAnimation::_ZFP_ZFAnimation_aniReadyStop(void)
{
    if(this->aniTarget() != zfnull)
    {
        _ZFP_I_ZFAnimationAniList *aniList = this->aniTarget()->objectTag<_ZFP_I_ZFAnimationAniList *>(_ZFP_I_ZFAnimationAniList::ClassData()->classNameFull());
        if(aniList != zfnull)
        {
            aniList->aniList.removeElement(this);
        }
    }
}
void ZFAnimation::_ZFP_ZFAnimation_aniDummyNotifyStop(void)
{
    d->aniDummyThreadId = zfidentityInvalid();
    this->aniImplNotifyStop();
}
void ZFAnimation::aniImplDelay(void)
{
    ++(d->aniDelayTaskId);
    d->aniDelayThreadId = ZFThreadExecuteInMainThreadAfterDelay(
        this->aniDelay(),
        ZF_GLOBAL_INITIALIZER_INSTANCE(ZFAnimationTaskHolder)->delayOnFinishListener,
        this->objectHolder(),
        ZFListenerData().param0(zflineAlloc(v_zfidentity, d->aniDelayTaskId))
        );
}
void ZFAnimation::aniImplDelayCancel(void)
{
    ++(d->aniDelayTaskId);
    ZFThreadExecuteCancel(d->aniDelayThreadId);
}
void ZFAnimation::aniImplDelayNotifyFinish(void)
{
    zfCoreAssertWithMessage(d->aniDelaying, "notify delay finish an animation which not delaying");
    d->aniDelaying = zffalse;
    this->aniImplStart();
}

void ZFAnimation::aniImplStart(void)
{
    // start a dummy animation if not implemented
    if(this->classData() == ZFAnimation::ClassData())
    {
        d->aniDummyThreadId = ZFThreadExecuteInMainThreadAfterDelay(
            this->aniDurationFixed(),
            ZF_GLOBAL_INITIALIZER_INSTANCE(ZFAnimationTaskHolder)->dummyOnFinishListener,
            this->objectHolder());
    }
}
void ZFAnimation::aniImplStop(void)
{
    if(this->classData() == ZFAnimation::ClassData())
    {
        ZFThreadExecuteCancel(d->aniDummyThreadId);
    }
}

void ZFAnimation::aniImplNotifyStop(void)
{
    zfCoreAssertWithMessage(d->aniRunning, "notify stop an animation which not started");
    this->_ZFP_ZFAnimation_aniReadyStop();
    ZFObject *aniTargetToRelease = this->aniTarget();

    d->aniRunning = zffalse;
    d->aniStopCalled = zffalse;
    this->aniOnStop();
    this->aniOnStopOrOnInvalid(zftrue);

    zfRelease(aniTargetToRelease);
    zfRelease(this);
}

ZF_NAMESPACE_GLOBAL_END

