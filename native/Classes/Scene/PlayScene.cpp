
#include "PlayScene.h"
#include "PlayScene.h"
#include "DialogManager.h"
#include "GameLayer.h"
#include "AlertDlg.h"
#include "GameEngine.h"
#include "LoadingScene.h"

PlayScene::PlayScene() : BaseScene()
{
}

PlayScene::~PlayScene()
{
}

bool PlayScene::init()
{
    if ( !BaseScene::init() )
    {
        return false;
    }
    return true;
}

void PlayScene::onEnter()
{
    BaseScene::onEnter();
    this->addChild(GameLayer::create());
}

void PlayScene::onExit()
{
    BaseScene::onExit();
}

/**
 * 局内返回键：返回主界面（不退出App）
 */
void PlayScene::keyBackClicked() {
    AlertDlg *pAlert = dynamic_cast<AlertDlg *>(DialogManager::shared()->getCurrentDialog());
    if (pAlert == nullptr) {
        auto alert = AlertDlg::create();
        alert->setAlertType(AlertDlg::ENUM_CONFIRM);
        alert->setCallback([]() {
            DialogManager::shared()->closeAllDialog();
            GameEngine::GetGameEngine()->resetGame();       //复位对局与积分，便于回主界面后重新开局
            Director::getInstance()->replaceScene(LoadingScene::create());
        }, []() {
            DialogManager::shared()->closeCurrentDialog();
        });
        alert->setText("本局游戏将直接结束且无法恢复，确定返回主界面？");
        DialogManager::shared()->showDialog(alert);
    } else {
        DialogManager::shared()->closeCurrentDialog();
    }
}

