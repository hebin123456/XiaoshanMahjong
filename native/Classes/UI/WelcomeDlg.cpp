#include "WelcomeDlg.h"
#include "PlayScene.h"
#include "SettingDlg.h"
#include "DialogManager.h"
#include "GameConfig.h"

using namespace std;

WelcomeDlg::WelcomeDlg() : IDialog()
{
    RegDialogCtrl("Button_Logon", m_btnLogon);
    m_pSexChoiceLayer = nullptr;
}

WelcomeDlg::~WelcomeDlg(){
}

void WelcomeDlg::onUILoaded() {
    m_btnLogon->addClickEventListener([this](Ref* sender) {
        //萧山麻将：点开局先选打牌语音性别，选完进入游戏
        showSexChoice();
    });
    //萧山麻将：主页设置按钮（规则开关、AI强度、起始积分、基本分、音量）
    ui::Button *pBtnSet = ui::Button::create("res/GameLayer/btn_action1.png");
    pBtnSet->setName("Btn_HomeSetting");
    pBtnSet->setTitleText("设置");
    pBtnSet->setTitleFontSize(26);
    pBtnSet->setTitleFontName("res/DFYuanW7-GB2312.ttf");
    pBtnSet->setScale(0.9f);
    pBtnSet->setPosition(Vec2(1160.0f, 100.0f));    //右下角
    pBtnSet->addClickEventListener([](Ref*) {
        DialogManager::shared()->showDialog(SettingDlg::create());
    });
    m_rootNode->addChild(pBtnSet, 10);
}

/**
 * 开局前选择打牌语音性别：男声开局 / 女声开局，选择后进入游戏（选择持久化）
 */
void WelcomeDlg::showSexChoice() {
    if (m_pSexChoiceLayer) { return; }    //防重复弹出

    const Size &sz = Director::getInstance()->getVisibleSize();
    const Vec2 &origin = Director::getInstance()->getVisibleOrigin();
    const Vec2 center = origin + sz * 0.5f;

    Node *pLayer = Node::create();
    m_pSexChoiceLayer = pLayer;
    this->addChild(pLayer, 100);

    //半透明罩铺满全屏并吞掉触摸，避免误点到底下按钮
    LayerColor *pMask = LayerColor::create(Color4B(0, 0, 0, 190));
    pMask->setContentSize(sz);
    pMask->setPosition(origin);
    pLayer->addChild(pMask, 0);
    auto swallow = EventListenerTouchOneByOne::create();
    swallow->setSwallowTouches(true);
    swallow->onTouchBegan = [](Touch *, Event *) -> bool { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(swallow, pMask);

    //标题
    Label *pTitle = Label::createWithTTF("选择打牌语音", "res/DFYuanW7-GB2312.ttf", 40);
    pTitle->setTextColor(Color4B(255, 235, 130, 255));
    pTitle->setPosition(center + Vec2(0.0f, 130.0f));
    pLayer->addChild(pTitle, 1);

    //男声/女声两个开局按钮
    const char *strNames[2] = {"男声开局", "女声开局"};
    for (int i = 0; i < 2; i++) {
        ui::Button *pBtn = ui::Button::create("res/GameLayer/btn_action1.png");
        pBtn->setTitleText(strNames[i]);
        pBtn->setTitleFontSize(30);
        pBtn->setTitleFontName("res/DFYuanW7-GB2312.ttf");
        pBtn->setScale(1.1f);
        pBtn->setPosition(center + Vec2(i == 0 ? -170.0f : 170.0f, 10.0f));
        pBtn->addClickEventListener([this, i](Ref*) {
            GameConfig *cfg = GameConfig::getInstance();
            cfg->m_iVoiceSex = (i == 0) ? 1 : 0;    //1男声 0女声
            cfg->saveConfig();
            if (m_pSexChoiceLayer) {
                m_pSexChoiceLayer->removeFromParent();
                m_pSexChoiceLayer = nullptr;
            }
            DialogManager::shared()->closeAllDialog();
            Director::getInstance()->replaceScene(PlayScene::create());
        });
        pLayer->addChild(pBtn, 1);
    }

    //取消
    Label *pCancel = Label::createWithTTF("取消", "res/DFYuanW7-GB2312.ttf", 28);
    pCancel->setTextColor(Color4B(200, 200, 200, 255));
    pCancel->setPosition(center + Vec2(0.0f, -120.0f));
    pLayer->addChild(pCancel, 1);
    auto cancelListener = EventListenerTouchOneByOne::create();
    cancelListener->setSwallowTouches(true);
    cancelListener->onTouchBegan = [pCancel](Touch *touch, Event *) -> bool {
        Vec2 p = pCancel->getParent()->convertToNodeSpace(touch->getLocation());
        return pCancel->getBoundingBox().containsPoint(p);
    };
    cancelListener->onTouchEnded = [this, pCancel](Touch *touch, Event *) {
        Vec2 p = pCancel->getParent()->convertToNodeSpace(touch->getLocation());
        if (pCancel->getBoundingBox().containsPoint(p)) {
            if (m_pSexChoiceLayer) {
                m_pSexChoiceLayer->removeFromParent();
                m_pSexChoiceLayer = nullptr;
            }
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(cancelListener, pCancel);
}
