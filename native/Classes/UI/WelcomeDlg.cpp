#include "WelcomeDlg.h"
#include "PlayScene.h"

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
}

