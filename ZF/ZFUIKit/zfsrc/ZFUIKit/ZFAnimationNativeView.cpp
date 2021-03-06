#include "ZFAnimationNativeView.h"
#include "protocol/ZFProtocolZFAnimationNativeView.h"

ZF_NAMESPACE_GLOBAL_BEGIN

ZFENUM_DEFINE(ZFAnimationNativeViewCurve)

// ============================================================
zfclassNotPOD _ZFP_ZFAnimationNativeViewPrivate
{
public:
    void *nativeAni;
    zfbool aniTargetAutoDisableCached;
    zfbool aniTargetEnableSaved;

public:
    _ZFP_ZFAnimationNativeViewPrivate(void)
    : nativeAni(zfnull)
    , aniTargetAutoDisableCached(zffalse)
    , aniTargetEnableSaved(zffalse)
    {
    }
};

// ============================================================
ZFOBJECT_REGISTER(ZFAnimationNativeView)

void ZFAnimationNativeView::objectOnInit(void)
{
    zfsuper::objectOnInit();
    d = zfpoolNew(_ZFP_ZFAnimationNativeViewPrivate);
    d->nativeAni = ZFPROTOCOL_ACCESS(ZFAnimationNativeView)->nativeAniCreate(this);
}
void ZFAnimationNativeView::objectOnDealloc(void)
{
    ZFPROTOCOL_ACCESS(ZFAnimationNativeView)->nativeAniDestroy(this, d->nativeAni);
    zfpoolDelete(d);
    d = zfnull;
    zfsuper::objectOnDealloc();
}

void ZFAnimationNativeView::objectInfoOnAppend(ZF_IN_OUT zfstring &ret)
{
    zfsuper::objectInfoOnAppend(ret);
    ret += "-";
    zfsFromPointerT(ret, this->nativeAnimation());
    ZFClassUtil::objectPropertyInfo(ret, this);
}
zfidentity ZFAnimationNativeView::objectHash(void)
{
    // no need to hash everything, for performance
    return zfidentityHash(zfsuper::objectHash()
            , this->aniCurve()
            , this->aniAlphaTo()
            , this->aniScaleXTo()
            , this->aniScaleYTo()
            , this->aniTranslateXTo()
            , this->aniTranslateYTo()
            , this->aniTranslatePixelXTo()
            , this->aniTranslatePixelYTo()
            , this->aniRotateXTo()
            , this->aniRotateYTo()
            , this->aniRotateZTo()
        );
}
ZFCompareResult ZFAnimationNativeView::objectCompare(ZF_IN ZFObject *anotherObj)
{
    if(zfsuper::ZFObject::objectCompare(anotherObj) != ZFCompareTheSame)
    {
        return ZFCompareUncomparable;
    }
    zfself *another = ZFCastZFObject(zfself *, anotherObj);
    if(another == zfnull) {return ZFCompareUncomparable;}

    if(zftrue
        && this->aniCurve() == another->aniCurve()
        && this->aniAlphaFrom() == another->aniAlphaFrom()
        && this->aniAlphaTo() == another->aniAlphaTo()
        && this->aniScaleXFrom() == another->aniScaleXFrom()
        && this->aniScaleXTo() == another->aniScaleXTo()
        && this->aniScaleYFrom() == another->aniScaleYFrom()
        && this->aniScaleYTo() == another->aniScaleYTo()
        && this->aniTranslateXFrom() == another->aniTranslateXFrom()
        && this->aniTranslateXTo() == another->aniTranslateXTo()
        && this->aniTranslateYFrom() == another->aniTranslateYFrom()
        && this->aniTranslateYTo() == another->aniTranslateYTo()
        && this->aniTranslatePixelXFrom() == another->aniTranslatePixelXFrom()
        && this->aniTranslatePixelXTo() == another->aniTranslatePixelXTo()
        && this->aniTranslatePixelYFrom() == another->aniTranslatePixelYFrom()
        && this->aniTranslatePixelYTo() == another->aniTranslatePixelYTo()
        && this->aniRotateXFrom() == another->aniRotateXFrom()
        && this->aniRotateXTo() == another->aniRotateXTo()
        && this->aniRotateYFrom() == another->aniRotateYFrom()
        && this->aniRotateYTo() == another->aniRotateYTo()
        && this->aniRotateZFrom() == another->aniRotateZFrom()
        && this->aniRotateZTo() == another->aniRotateZTo()
        )
    {
        return ZFCompareTheSame;
    }
    return ZFCompareUncomparable;
}

ZFMETHOD_DEFINE_0(ZFAnimationNativeView, void *, nativeAnimation)
{
    return d->nativeAni;
}

// ============================================================
// start stop
zfbool ZFAnimationNativeView::aniImplCheckValid(void)
{
    return (zfsuper::aniImplCheckValid()
        && this->aniTarget() != zfnull && this->aniTarget()->classData()->classIsTypeOf(ZFUIView::ClassData())
    );
}

void ZFAnimationNativeView::aniOnStart(void)
{
    zfsuper::aniOnStart();
    ZFUIView *aniTarget = ZFAny(this->aniTarget());
    if(aniTarget == zfnull)
    {
        d->aniTargetAutoDisableCached = zffalse;
    }
    else
    {
        d->aniTargetAutoDisableCached = this->aniTargetAutoDisable();
        if(d->aniTargetAutoDisableCached)
        {
            d->aniTargetEnableSaved = aniTarget->viewUIEnableTree();
            aniTarget->viewUIEnableTree(zffalse);
        }
    }
}
void ZFAnimationNativeView::aniOnStop(void)
{
    ZFUIView *aniTarget = ZFAny(this->aniTarget());
    if(aniTarget != zfnull && d->aniTargetAutoDisableCached)
    {
        aniTarget->viewUIEnableTree(d->aniTargetAutoDisableCached);
    }
    zfsuper::aniOnStop();
}

void ZFAnimationNativeView::aniImplStart(void)
{
    zfsuper::aniImplStart();
    ZFUIView *aniTarget = ZFAny(this->aniTarget());
    {
        ZFUIView *parent = aniTarget;
        while(parent->viewParent() != zfnull)
        {
            parent = parent->viewParent();
        }
        parent->layoutIfNeed();
    }
    ZFPROTOCOL_ACCESS(ZFAnimationNativeView)->nativeAniStart(this,
        aniTarget != zfnull ? aniTarget->scaleFixed() : 1.0f);
}
void ZFAnimationNativeView::aniImplStop(void)
{
    ZFPROTOCOL_ACCESS(ZFAnimationNativeView)->nativeAniStop(this);
    zfsuper::aniImplStop();
}

ZF_NAMESPACE_GLOBAL_END

