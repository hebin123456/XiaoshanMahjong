#include "WelcomeDlg.h"
#include "PlayScene.h"
#include "SettingDlg.h"
#include "DialogManager.h"

using namespace std;

WelcomeDlg::WelcomeDlg() : IDialog()
{
    RegDialogCtrl("Button_Logon", m_btnLogon);
}

WelcomeDlg::~WelcomeDlg(){
}

void WelcomeDlg::onUILoaded() {
    m_btnLogon->addClickEventListener([this](Ref* sender) {
        //萧山麻将：屏蔽欢迎提示弹窗，点击开局直接进入游戏
        DialogManager::shared()->closeAllDialog();
        Director::getInstance()->replaceScene(PlayScene::create());
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

