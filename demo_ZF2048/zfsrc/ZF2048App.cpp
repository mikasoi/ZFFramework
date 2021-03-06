#include "ZF2048App.h"
#include "ZFUIWidget.h"

#include "ZF2048App/ZF2048AppButton.h"
#include "ZF2048App/ZF2048AppAutoMoveSettingDialog.h"
#include "ZF2048App/ZF2048AppAutoMoveRunner.h"
#include "ZF2048App/ZF2048AppSettingDialog.h"

ZF_NAMESPACE_GLOBAL_BEGIN

zfclass _ZFP_ZF2048AppTextView : zfextends ZFUITextView
{
    ZFOBJECT_DECLARE(_ZFP_ZF2048AppTextView, ZFUITextView)

protected:
    zfoverride
    virtual void objectOnInitFinish(void)
    {
        zfsuper::objectOnInitFinish();
        this->textColor(ZFUIColorBlue());
    }
};

zfclass _ZFP_ZF2048AppPrivate : zfextends ZFObject
{
    ZFOBJECT_DECLARE(_ZFP_ZF2048AppPrivate, ZFObject)

public:
    ZF2048App *owner;

    ZFPROPERTY_RETAIN_READONLY(ZFUILinearLayout *, mainLayout, zflineAlloc(ZFUILinearLayout))
    ZFPROPERTY_RETAIN_READONLY(ZFUILinearLayout *, headerLayout, zflineAlloc(ZFUILinearLayout))

    ZFPROPERTY_RETAIN_READONLY(ZFUIButtonBasic *, autoMoveButton, zflineAlloc(ZF2048AppButton))
    ZFPROPERTY_RETAIN_READONLY(ZFUIButtonBasic *, undoButton, zflineAlloc(ZF2048AppButton))
    ZFPROPERTY_RETAIN_READONLY(ZFUIViewLayout *, scoreLayoutHolder, zflineAlloc(ZFUIViewLayout))
    ZFPROPERTY_RETAIN_READONLY(ZFUILinearLayout *, scoreLayout, zflineAlloc(ZFUILinearLayout))
    ZFPROPERTY_RETAIN_READONLY(ZFUITextView *, scoreHint, zflineAlloc(_ZFP_ZF2048AppTextView))
    ZFPROPERTY_RETAIN_READONLY(ZFUITextView *, scoreLabel, zflineAlloc(_ZFP_ZF2048AppTextView))
    ZFPROPERTY_RETAIN_READONLY(ZFUIButtonBasic *, settingButton, zflineAlloc(ZF2048AppButton))
    ZFPROPERTY_RETAIN_READONLY(ZFUIButtonBasic *, aboutButton, zflineAlloc(ZF2048AppButton))

    ZFPROPERTY_RETAIN_READONLY(ZF2048AppAutoMoveSettingDialog *, autoMoveSettingDialog, zflineAlloc(ZF2048AppAutoMoveSettingDialog))
    ZFPROPERTY_RETAIN_READONLY(ZF2048AppAutoMoveRunner *, autoMoveRunner, zflineAlloc(ZF2048AppAutoMoveRunner))

public:
    void orientationUpdate(void)
    {
        ZFUIWindow *window = ZFUIWindow::windowForView(this->mainLayout());
        ZFUIOrientationEnum orientation = ZFUIOrientation::e_Top;
        if(window != zfnull)
        {
            orientation = window->windowOwnerSysWindow()->sysWindowOrientation();
        }
        else
        {
            orientation = ZFUISysWindow::mainWindow()->sysWindowOrientation();
        }
        switch(orientation)
        {
            case ZFUIOrientation::e_Top:
            case ZFUIOrientation::e_Bottom:
                this->mainLayout()->layoutOrientation(ZFUIOrientation::e_Top);
                this->headerLayout()->layoutOrientation(ZFUIOrientation::e_Left);
                this->headerLayout()->layoutParam()->sizeParam(ZFUISizeParamFillWrap());
                this->scoreLayout()->layoutOrientation(ZFUIOrientation::e_Left);
                break;
            case ZFUIOrientation::e_Left:
            case ZFUIOrientation::e_Right:
                this->mainLayout()->layoutOrientation(ZFUIOrientation::e_Left);
                this->headerLayout()->layoutOrientation(ZFUIOrientation::e_Top);
                this->headerLayout()->layoutParam()->sizeParam(ZFUISizeParamWrapFill());
                this->scoreLayout()->layoutOrientation(ZFUIOrientation::e_Top);
                break;
            default:
                zfCoreCriticalShouldNotGoHere();
                return ;
        }
    }
    ZFLISTENER_INLINE(orientationOnChange)
    {
        if(ZFUIWindow::windowForView(this->mainLayout())->windowOwnerSysWindow() != listenerData.sender())
        {
            return ;
        }
        this->orientationUpdate();
    }

public:
    ZFLISTENER_INLINE(appPaused)
    {
        this->autoMoveRunner()->runnerStop();
        this->autoMoveButton()->buttonChecked(zffalse);
    }

public:
    void gameOver(void)
    {
        zfblockedAlloc(ZF2048AppDialog, hintDialog);
        hintDialog->styleKeyForProperty("dialogTitle", "ZF2048_gameOver");

        zfblockedAlloc(ZFUILinearLayout, container);
        hintDialog->dialogContent(container);
        container->layoutOrientation(ZFUIOrientation::e_Left);
        container->layoutChildSpace(ZFUIGlobalStyle::DefaultStyle()->itemSpace());

        zfblockedAlloc(ZFObject, userData);
        userData->objectTag("dialog", hintDialog->objectHolder());
        userData->objectTag("game", this->owner->game()->objectHolder());

        zfblockedAlloc(ZF2048AppButton, retryButton);
        container->childAdd(retryButton);
        retryButton->buttonLabelStyleNormal()->styleKeyForProperty("text", "ZF2048_gameOver_retry");
        ZFLISTENER_LOCAL(retryButtonOnClick, {
            ZF2048AppDialog *dialog = userData->objectTag("dialog")->objectHolded();
            ZF2048Game *game = userData->objectTag("game")->objectHolded();
            dialog->dialogHide();
            game->gameReset(game->gameDataWidth(), game->gameDataHeight());
            game->gameFocus();
        })
        retryButton->observerAdd(ZFUIButton::EventButtonOnClick(), retryButtonOnClick, userData);

        zfblockedAlloc(ZF2048AppButton, cancelButton);
        container->childAdd(cancelButton);
        cancelButton->buttonLabelStyleNormal()->styleKeyForProperty("text", "ZF2048_gameOver_cancel");
        ZFLISTENER_LOCAL(cancelButtonOnClick, {
            ZF2048AppDialog *dialog = userData->objectTag("dialog")->objectHolded();
            dialog->dialogHide();

            ZF2048Game *game = userData->objectTag("game")->objectHolded();
            game->gameFocus();
        })
        cancelButton->observerAdd(ZFUIButton::EventButtonOnClick(), cancelButtonOnClick, userData);

        hintDialog->dialogShow();
    }

public:
    void dataUpdate(void)
    {
        this->scoreLabel()->text(zfintToString(this->owner->game()->gameScore()));
        if(!this->owner->game()->gameCanMove())
        {
            this->gameOver();
        }
    }
    ZFLISTENER_INLINE(dataOnChange)
    {
        this->dataUpdate();
    }

public:
    ZFLISTENER_INLINE(dialogAfterHide)
    {
        this->owner->game()->gameFocus();
    }

    ZFLISTENER_INLINE(autoMoveOnClick)
    {
        if(this->autoMoveButton()->buttonChecked())
        {
            this->autoMoveButton()->buttonChecked(zffalse);
            this->autoMoveSettingDialog()->dialogShow();
        }
        else
        {
            this->autoMoveRunner()->runnerStop();
            this->owner->game()->gameFocus();
        }
    }
    ZFLISTENER_INLINE(autoMoveOnStart)
    {
        this->autoMoveButton()->buttonChecked(zftrue);
        this->autoMoveRunner()->actionList.removeAll();
        this->autoMoveRunner()->actionList.copyFrom(this->autoMoveSettingDialog()->autoMoves);
        this->autoMoveRunner()->runnerStart();
    }
    ZFLISTENER_INLINE(autoMoveOnStop)
    {
        this->autoMoveButton()->buttonChecked(zffalse);
    }
    ZFLISTENER_INLINE(autoMoveStop)
    {
        this->autoMoveRunner()->runnerStop();
    }
    ZFLISTENER_INLINE(settingOnChange)
    {
        ZF2048AppSettingDialog *settingDialog = listenerData.sender()->toAny();
        this->owner->game()->gameReset(settingDialog->dataWidth, settingDialog->dataHeight);
    }
    ZFLISTENER_INLINE(settingOnClick)
    {
        zfblockedAlloc(ZF2048AppSettingDialog, settingDialog, this->owner->game()->gameDataWidth(), this->owner->game()->gameDataHeight());
        settingDialog->observerAdd(ZF2048AppSettingDialog::EventSettingOnChange(), ZFCallbackForMemberMethod(this, ZFMethodAccess(zfself, settingOnChange)));
        settingDialog->observerAdd(ZF2048AppSettingDialog::EventDialogAfterHide(), ZFCallbackForMemberMethod(this, ZFMethodAccess(zfself, dialogAfterHide)));
        settingDialog->dialogShow();
    }
public:
    void setupChildren(void)
    {
        this->owner->childAdd(this->mainLayout());
        this->mainLayout()->layoutParam()->sizeParam(ZFUISizeParamFillFill());
        this->mainLayout()->layoutChildSpace(ZFUIGlobalStyle::DefaultStyle()->itemSpace());

        this->mainLayout()->childAdd(this->headerLayout());
        this->headerLayout()->layoutChildSpace(ZFUIGlobalStyle::DefaultStyle()->itemSpace());

        this->headerLayout()->childAdd(this->autoMoveButton());
        this->autoMoveButton()->buttonCheckable(zftrue);
        this->autoMoveButton()->buttonBackgroundStyleChecked()->image(this->autoMoveButton()->buttonBackgroundStyleNormal()->image());

        this->headerLayout()->childAdd(this->undoButton());

        this->headerLayout()->childAdd(this->scoreLayoutHolder());
        this->scoreLayoutHolder()->layoutParam<ZFUILinearLayoutParam *>()->layoutWeight(1);

        this->scoreLayoutHolder()->childAdd(this->scoreLayout());
        this->scoreLayout()->layoutParam()->layoutAlign(ZFUIAlign::e_Center);
        this->scoreLayout()->childAdd(this->scoreHint());

        this->scoreHint()->viewSizeMin(ZFUISizeMake(ZFUIGlobalStyle::DefaultStyle()->itemSizeText()));
        this->scoreHint()->textAlign(ZFUIAlign::e_Center);

        this->scoreLayout()->childAdd(this->scoreLabel());
        this->scoreLabel()->viewSizeMin(ZFUISizeMake(ZFUIGlobalStyle::DefaultStyle()->itemSizeText()));
        this->scoreLabel()->textAlign(ZFUIAlign::e_Center);

        this->headerLayout()->childAdd(this->settingButton());
        this->headerLayout()->childAdd(this->aboutButton());

        this->mainLayout()->childAdd(this->owner->game());
        this->owner->game()->layoutParam()->sizeParam(ZFUISizeParamFillFill());
    }
    void setupSetting_autoMove(void)
    {
        this->autoMoveButton()->observerAdd(ZFUIButton::EventButtonOnClick(), ZFCallbackForMemberMethod(this, ZFMethodAccess(zfself, autoMoveOnClick)));

        this->autoMoveSettingDialog()->observerAdd(ZF2048AppAutoMoveSettingDialog::EventAutoMoveSettingOnChange(), ZFCallbackForMemberMethod(this, ZFMethodAccess(zfself, autoMoveOnStart)));
        this->autoMoveSettingDialog()->observerAdd(ZF2048AppAutoMoveSettingDialog::EventDialogAfterHide(), ZFCallbackForMemberMethod(this, ZFMethodAccess(zfself, dialogAfterHide)));

        this->autoMoveRunner()->game = this->owner->game();
        this->autoMoveRunner()->observerAdd(ZF2048AppAutoMoveRunner::EventRunnerOnStop(), ZFCallbackForMemberMethod(this, ZFMethodAccess(zfself, autoMoveOnStop)));

        this->autoMoveButton()->buttonLabelStyleNormal()->styleKeyForProperty("text", "ZF2048_autoMove");
        this->autoMoveButton()->buttonLabelStyleChecked()->styleKeyForProperty("text", "ZF2048_autoMoveStop");
    }
    void setupSetting_undo(void)
    {
        ZFLISTENER_LOCAL(undoOnClick, {
            ZF2048App *owner = userData->objectHolded();
            owner->game()->gameUndo();
            owner->game()->gameFocus();
        })
        this->undoButton()->observerAdd(ZFUIButton::EventButtonOnClick(), undoOnClick, this->owner->objectHolder());
        this->undoButton()->observerAdd(ZFUIButton::EventButtonOnClick(), ZFCallbackForMemberMethod(this, ZFMethodAccess(zfself, autoMoveStop)));

        this->undoButton()->buttonLabelStyleNormal()->styleKeyForProperty("text", "ZF2048_undo");
    }
    void setupSetting_setting(void)
    {
        this->settingButton()->observerAdd(ZFUIButton::EventButtonOnClick(), ZFCallbackForMemberMethod(this, ZFMethodAccess(zfself, settingOnClick)));
        this->settingButton()->observerAdd(ZFUIButton::EventButtonOnClick(), ZFCallbackForMemberMethod(this, ZFMethodAccess(zfself, autoMoveStop)));

        this->settingButton()->buttonLabelStyleNormal()->styleKeyForProperty("text", "ZF2048_setting");
    }
    void setupSetting_about(void)
    {
        ZFLISTENER_LOCAL(aboutOnClick, {
            _ZFP_ZF2048AppPrivate::showAbout();

            ZF2048App *owner = userData->objectHolded();
            owner->game()->gameFocus();
        })
        this->aboutButton()->observerAdd(ZFUIButton::EventButtonOnClick(), aboutOnClick, this->owner->objectHolder());
        this->aboutButton()->observerAdd(ZFUIButton::EventButtonOnClick(), ZFCallbackForMemberMethod(this, ZFMethodAccess(zfself, autoMoveStop)));

        this->aboutButton()->buttonLabelStyleNormal()->styleKeyForProperty("text", "ZF2048_about");
    }
public:
    static void showAbout(void)
    {
        ZFUIHintShow(ZFStyleGet("ZF2048_aboutContent").to<v_zfstring *>()->zfv);
    }

protected:
    virtual void objectOnInit(ZF_IN ZF2048App *owner)
    {
        this->objectOnInit();
        this->owner = owner;
    }
    zfoverride
    virtual void objectOnInit(void)
    {
        zfsuper::objectOnInit();
    }
    zfoverride
    virtual void objectOnInitFinish(void)
    {
        zfsuper::objectOnInitFinish();

        this->setupChildren();

        this->setupSetting_autoMove();
        this->setupSetting_undo();
        this->setupSetting_setting();
        this->setupSetting_about();
        this->scoreHint()->styleKeyForProperty("text", "ZF2048_score");

        this->owner->game()->observerAdd(ZF2048Game::EventGameDataOnChange(), ZFCallbackForMemberMethod(this, ZFMethodAccess(zfself, dataOnChange)));

        ZFObjectGlobalEventObserver().observerAdd(ZFObserverAddParam()
                .eventId(ZFUISysWindow::EventSysWindowOnRotate())
                .observer(ZFCallbackForMemberMethod(this, ZFMethodAccess(zfself, orientationOnChange)))
                .owner(this)
            );

        ZFObjectGlobalEventObserver().observerAdd(ZFUISysWindow::EventSysWindowOnPause(), ZFCallbackForMemberMethod(this, ZFMethodAccess(zfself, appPaused)));

        this->dataUpdate();
        this->orientationUpdate();
    }
    zfoverride
    virtual void objectOnDealloc(void)
    {
        ZFObjectGlobalEventObserver().observerRemoveByOwner(this);
        zfsuper::objectOnDealloc();
    }
};

void ZF2048App::objectOnInit(void)
{
    zfsuper::objectOnInit();
    d = zfAlloc(_ZFP_ZF2048AppPrivate, this);
}
void ZF2048App::objectOnDealloc(void)
{
    zfRetainChange(d, zfnull);
    zfsuper::objectOnDealloc();
}

ZF_NAMESPACE_GLOBAL_END

