//
// Created by farmer on 2018/7/4.
//

#include <memory>

#include "GameLayer.h"
#include "AIPlayer.h"
#include "RealPlayer.h"
#include "AIEngine.h"
#include "SettingDlg.h"
#include "GameConfig.h"
#include "AlertDlg.h"
#include "DialogManager.h"
#include "LoadingScene.h"

static uint8_t USER_OP_MAX_TIME = 30; // 出牌倒计时秒数

GameLayer::GameLayer()  : IDialog() {
    RegDialogCtrl("Button_Exit", m_btnExit);
    RegDialogCtrl("Button_Set", m_btnSetting);
    RegDialogCtrl("OperateNotifyGroup", m_pOperateNotifyGroup);
    RegDialogCtrl("Text_LeftCard", m_pTextCardNum);
    for (int i = 0; i < GAME_PLAYER; i++) {
        //初始化头像节点数组
        RegDialogCtrl(utility::toString("face_frame_", i), m_FaceFrame[i]);
        RegDialogCtrl(utility::toString("PlayerPanel_", i), m_PlayerPanel[i]);
    }
    m_GameEngine = GameEngine::GetGameEngine();  //构造游戏引擎
    m_bOperate = false;
    m_bMove = false;
    m_pOperateNotifyGroup = NULL;
    m_pTextCardNum = NULL;
    m_pCardWall = NULL;
    m_iWallFirstSlot = -1;
    m_bDiceConfirmed = false;
    m_pOutCard = NULL;
    m_iOutCardTimeOut = USER_OP_MAX_TIME;
    m_MeChairID = 0;
    initGame();
}

GameLayer::~GameLayer() {
}

void GameLayer::onUILoaded() {
    m_btnExit->addClickEventListener([this](Ref* sender) {
        // 退出游戏按钮：返回主界面（不退出App）
        auto alert = AlertDlg::create();
        alert->setAlertType(AlertDlg::ENUM_CONFIRM);
        alert->setCallback([]() {
            DialogManager::shared()->closeAllDialog();
            GameEngine::GetGameEngine()->resetGame();       //复位对局与积分，便于回主界面后重新开局
            Director::getInstance()->replaceScene(LoadingScene::create());
        }, []() {
            DialogManager::shared()->closeAllDialog();
        });
        alert->setText("本局游戏将直接结束且无法恢复，确定返回主界面？");
        DialogManager::shared()->showDialog(alert);
    });
    m_btnSetting->addClickEventListener([this](Ref* sender) {
        // 游戏设置按钮（局内模式：仅音量 + 「！本局信息」，不允许改规则）
        DialogManager::shared()->showDialog(SettingDlg::create(true));
    });
    //弃牌区位置微调（为桌面四边牌墙让位）
    Node *pDiscardCard0 = this->getChildByName("DiscardCard_0");
    if (pDiscardCard0) {
        pDiscardCard0->setPosition(Vec2(430.0f, 182.0f));
    }
    Node *pDiscardCard2 = this->getChildByName("DiscardCard_2");
    if (pDiscardCard2) {
        pDiscardCard2->setPosition(Vec2(430.0f, 448.0f));
    }
    // 玩家加入游戏（语音性别取开局时的选择：0女声 1男声）
    RealPlayer *pIPlayer = new RealPlayer(
            GameConfig::getInstance()->m_iVoiceSex == 0 ? IPlayer::FEMALE : IPlayer::MALE, this);
    m_GameEngine->onUserEnter(pIPlayer);
    m_MeChairID = pIPlayer->getChairID();
    // 创建个定时任务，用来添加机器人
    schedule([this](float) {
        //机器人玩家加入游戏，返回false说明已经满了，随机生成性别
        if (!m_GameEngine->onUserEnter(new AIPlayer(time(NULL) % 2 == 0 ? IPlayer::MALE : IPlayer::FEMALE, new AIEngine))) {
            unschedule("AiEnterGame");//人满，关闭定时任务
        };
    }, 1.0f, "AiEnterGame");
}

/**
 * 初始化游戏变量
 */
void GameLayer::initGame() {
    m_cbLeftCardCount = 0;
    m_cbBankerChair = INVALID_CHAIR;
    m_iWallFirstSlot = -1;
    m_bDiceConfirmed = false;
    m_iBankerDiceStep = 0;
    memset(&m_cbWeaveItemCount, 0, sizeof(m_cbWeaveItemCount));
    memset(&m_cbDiscardCount, 0, sizeof(m_cbDiscardCount));
    for (uint8_t i = 0; i < GAME_PLAYER; i++) {
        memset(m_cbCardIndex[i], 0, sizeof(m_cbCardIndex[i]));
        memset(m_WeaveItemArray[i], 0, sizeof(m_WeaveItemArray[i]));
        memset(m_cbDiscardCard[i], 0, sizeof(m_cbDiscardCard[i]));
    }
}

bool GameLayer::onUserEnterEvent(IPlayer *pIPlayer) {
    if (m_GameEngine->getPlayerCount() >= GAME_PLAYER) {
        unschedule("AiEnterGame");   //人满，关闭ai加入任务
    }
    if (pIPlayer && pIPlayer->getChairID() < GAME_PLAYER) {
        uint8_t uChairID = pIPlayer->getChairID();
        // 显示头像
        ui::ImageView *pImageHeader = dynamic_cast<ui::ImageView *>(UIHelper::seekNodeByName(m_FaceFrame[uChairID], "Image_Header"));  //头像
        ui::Text *pTextScore = dynamic_cast<ui::Text *>(UIHelper::seekNodeByName(m_FaceFrame[uChairID], "Text_Score"));  //头像
        pTextScore->setString(utility::toString(m_GameEngine->getGameScoreTable()[uChairID]));     //显示总局积分（开局每人50分）
        pImageHeader->loadTexture(utility::toString("res/GameLayer/im_defaulthead_", pIPlayer->getSexAsInt(), ".png"));    //设置头像
    }
    return true;
}

bool GameLayer::onGameStartEvent(CMD_S_GameStart GameStart) {
    cocos2d::log("接收到游戏开始事件");   //接收到游戏开始事件
    initGame();
    removeDiceNodes();    //清理上一局遗留的骰子/掷骰按钮
    //调整头像位置
    m_FaceFrame[0]->runAction(MoveTo::create(0.50f, Vec2(0080.00f, 250.00f)));
    m_FaceFrame[1]->runAction(MoveTo::create(0.50f, Vec2(0080.00f, 380.00f)));
    m_FaceFrame[2]->runAction(MoveTo::create(0.50f, Vec2(1060.00f, 640.00f)));
    m_FaceFrame[3]->runAction(MoveTo::create(0.50f, Vec2(1200.00f, 380.00f)));
    //剩余的牌：掷骰（定庄/定位）完成前一张未摸，牌墙完整显示136张；发牌后才显示真实剩余数
    m_cbLeftCardCount = MAX_REPERTORY;
    m_cbBankerChair = GameStart.cbBankerUser;
    cocos2d::log("本局财神（发牌后翻出的那张牌）：%02x，骰子：%d+%d", GameStart.cbJokerCard, GameStart.cbDice1, GameStart.cbDice2);
    GameLogic::switchToCardIndex(GameStart.cbCardData, MAX_COUNT - 1, m_cbCardIndex[m_MeChairID]);
    //界面显示
    m_pTextCardNum->setString(utility::toString((int) m_cbLeftCardCount));
    showAndUpdateUserScore(m_GameEngine->getGameScoreTable());   //刷新总局积分显示
    showCardWall(GameStart.cbBankerUser, GameStart.cbDice1, GameStart.cbDice2);    //绘制四边牌墙（按骰子留出起牌缺口）
    showBankerStreakBadge();    //庄家头像角标：庄 / N连庄
    //萧山麻将掷骰：首局每人掷骰定庄（AI自动，真人按钮）；非首局庄家掷骰定起牌位置
    uint8_t cbBankerView = m_GameEngine->switchViewChairID(GameStart.cbBankerUser, m_MeChairID);
    cocos2d::log("掷骰分支：首局=%d 庄家椅子%d 庄家视图%d 我椅子%d",
                 GameStart.bFirstGame ? 1 : 0, GameStart.cbBankerUser, cbBankerView, m_MeChairID);
    if (GameStart.bFirstGame) {
        showBankerDiceFlow(GameStart);
    } else if (cbBankerView == 0 || GameStart.cbBankerUser == m_MeChairID) {
        cocos2d::log("玩家坐庄：显示掷骰子按钮等待点击");
        showDiceRollButton(GameStart);
    } else {
        this->scheduleOnce([this, GameStart](float) {
            playDiceAnimation(GameStart);
        }, 0.5f, "AiAutoDice");
    }
    return true;
}

/**
 * 发牌事件
 * @param SendCard
 * @return
 */
bool GameLayer::onSendCardEvent(CMD_S_SendCard SendCard) {
    m_iOutCardTimeOut = USER_OP_MAX_TIME; //重置计时器
    m_cbLeftCardCount--;
    popCardWallOne();                    //摸一张牌，牌墙起牌口少一张
    if (SendCard.cbCurrentUser == m_MeChairID) {
        m_cbCardIndex[m_MeChairID][GameLogic::switchToCardIndex(SendCard.cbCardData)]++;
    }
    return showSendCard(SendCard);
}

/**
 * 出牌事件
 * @param OutCard
 * @return
 */
bool GameLayer::onOutCardEvent(CMD_S_OutCard OutCard) {
    if (OutCard.cbOutCardUser == m_MeChairID) {
        m_cbCardIndex[m_MeChairID][GameLogic::switchToCardIndex(OutCard.cbOutCardData)]--;
    }
    m_cbDiscardCard[OutCard.cbOutCardUser][m_cbDiscardCount[OutCard.cbOutCardUser]++] = OutCard.cbOutCardData;
    uint8_t cbViewID = m_GameEngine->switchViewChairID(OutCard.cbOutCardUser, m_MeChairID);
    cocostudio::timeline::ActionTimeline *action = CSLoader::createTimeline("res/SignAnim.csb");
    action->gotoFrameAndPlay(0, 10, true);
    Node *pSignNode = CSLoader::createNode("res/SignAnim.csb");
    pSignNode->setName("SignAnim");
    std::vector<Node *> aChildren;
    aChildren.clear();
    UIHelper::getChildren(m_rootNode, "SignAnim", aChildren);
    for (auto &subWidget : aChildren) {
        subWidget->removeFromParent();
    }
    ui::ImageView *pRecvCard = dynamic_cast<ui::ImageView *>(UIHelper::seekNodeByName(m_PlayerPanel[cbViewID], utility::toString("RecvCard_", (int) cbViewID)));
    if (pRecvCard) {
        pRecvCard->setVisible(false);
    }
    switch (cbViewID) {
        case 0: {
            showAndUpdateHandCard();                     //更新手上的牌
            ui::Layout *pRecvCardList = dynamic_cast<ui::Layout *>(UIHelper::seekNodeByName(m_PlayerPanel[cbViewID], utility::toString("RecvHandCard_0")));
            pRecvCardList->removeAllChildren(); //移除出牌位置的牌
            ui::Layout *pDiscardCard0 = dynamic_cast<ui::Layout *>(this->getChildByName( "DiscardCard_0" ));//显示出的牌
            pDiscardCard0->removeAllChildren();
            uint8_t bDiscardCount = m_cbDiscardCount[OutCard.cbOutCardUser]; //12
            float x = 0;
            float y = 0;
            for (int i = 0; i < bDiscardCount; i++) {
                uint8_t col = static_cast<uint8_t>(i % 12);
                uint8_t row = static_cast<uint8_t>(i / 12);
                x = (col == 0) ? 0 : x;  //X复位
                y = row * 90;
                ui::ImageView *pCard0 = UIHelper::createDiscardCardImageView(cbViewID, m_cbDiscardCard[OutCard.cbOutCardUser][i]);
                pCard0->setAnchorPoint(Vec2(0, 0));
                pCard0->setPosition(Vec2(x, y));
                pCard0->setLocalZOrder(10 - row);
                x += 76;
                if (i == bDiscardCount - 1) {   //最后一张，添加动画效果
                    pSignNode->setAnchorPoint(Vec2(0.5, 0.5));
                    pSignNode->setPosition(Vec2(39, 110));
                    pCard0->addChild(pSignNode);
                    pSignNode->runAction(action);
                }
                pDiscardCard0->addChild(pCard0);
            }
            pDiscardCard0->setScale(0.45f, 0.45f);
            //听牌判断
            uint8_t cbWeaveItemCount = m_cbWeaveItemCount[OutCard.cbOutCardUser];
            tagWeaveItem *pTagWeaveItem = m_WeaveItemArray[OutCard.cbOutCardUser];
            uint8_t *cbCardIndex = m_cbCardIndex[OutCard.cbOutCardUser];
            showTingResult(cbCardIndex, pTagWeaveItem, cbWeaveItemCount);
            break;
        }
        case 1: {
            //显示出的牌
            ui::Layout *pDiscardCard1 = dynamic_cast<ui::Layout *>(this->getChildByName( "DiscardCard_1" ));
            pDiscardCard1->removeAllChildren();
            uint8_t bDiscardCount = m_cbDiscardCount[OutCard.cbOutCardUser]; //
            float x = 0;
            float y = 0;
            for (int i = 0; i < bDiscardCount; i++) {
                uint8_t col = static_cast<uint8_t>(i % 11);
                uint8_t row = static_cast<uint8_t>(i / 11);
                y = (col == 0) ? 0 : y;  //X复位
                x = 116 * row;
                ui::ImageView *pCard1 = UIHelper::createDiscardCardImageView(cbViewID, m_cbDiscardCard[OutCard.cbOutCardUser][i]);
                pCard1->setAnchorPoint(Vec2(0, 0));
                pCard1->setPosition(Vec2(x, 740 - y));
                pCard1->setLocalZOrder(col);
                y += 74;
                if (i == bDiscardCount - 1) {   //最后一张，添加动画效果
                    pSignNode->setAnchorPoint(Vec2(0.5, 0.5));
                    pSignNode->setPosition(Vec2(81, 110));
                    pCard1->addChild(pSignNode);
                    pSignNode->runAction(action);
                }
                pDiscardCard1->addChild(pCard1);
            }
            pDiscardCard1->setScale(0.5, 0.5);
            break;
        }
        case 2: {
            ui::Layout *pDiscardCard2 = dynamic_cast<ui::Layout *>(this->getChildByName( "DiscardCard_2" ));
            pDiscardCard2->removeAllChildren();
            uint8_t bDiscardCount = m_cbDiscardCount[OutCard.cbOutCardUser]; //12
            float x = 0;
            float y = 0;
            for (int i = 0; i < bDiscardCount; i++) {
                uint8_t col = static_cast<uint8_t>(i % 12);
                uint8_t row = static_cast<uint8_t>(i / 12);
                x = (col == 0) ? 0 : x;  //X复位
                y = 90 - row * 90;
                ui::ImageView *pCard2 = UIHelper::createDiscardCardImageView(cbViewID, m_cbDiscardCard[OutCard.cbOutCardUser][i]);
                pCard2->setAnchorPoint(Vec2(0, 0));
                pCard2->setPosition(Vec2(x, y));
                pCard2->setLocalZOrder(row);
                x += 76;
                if (i == bDiscardCount - 1) {   //最后一张，添加动画效果
                    pSignNode->setAnchorPoint(Vec2(0.5, 0.5));
                    pSignNode->setPosition(Vec2(39, 59));
                    pCard2->addChild(pSignNode);
                    pSignNode->runAction(action);
                }
                pDiscardCard2->addChild(pCard2);
            }
            pDiscardCard2->setScale(0.45f, 0.45f);

            break;
        }
        case 3: {
            //显示出的牌
            ui::Layout *pDiscardCard3 = dynamic_cast<ui::Layout *>(this->getChildByName( "DiscardCard_3" ));
            pDiscardCard3->removeAllChildren();
            uint8_t bDiscardCount = m_cbDiscardCount[OutCard.cbOutCardUser]; //
            float x = 0;
            float y = 0;
            for (int i = 0; i < bDiscardCount; i++) {
                uint8_t col = static_cast<uint8_t>(i % 11);
                uint8_t row = static_cast<uint8_t>(i / 11);
                y = (col == 0) ? 0 : y;  //X复位
                x = 240 - (116 * row);
                ui::ImageView *pCard3 = UIHelper::createDiscardCardImageView(cbViewID, m_cbDiscardCard[OutCard.cbOutCardUser][i]);
                pCard3->setAnchorPoint(Vec2(0, 0));
                pCard3->setPosition(Vec2(x, y));
                pCard3->setLocalZOrder(20 - col);
                y += 74;
                if (i == bDiscardCount - 1) {   //最后一张，添加动画效果
                    pSignNode->setAnchorPoint(Vec2(0.5, 0.5));
                    pSignNode->setPosition(Vec2(39, 110));
                    pCard3->addChild(pSignNode);
                    pSignNode->runAction(action);
                }
                pDiscardCard3->addChild(pCard3);
            }
            pDiscardCard3->setScale(0.5, 0.5);
            break;
        }
        default:
            break;
    }
    for (int j = 0; j < GAME_PLAYER; ++j) {   //发牌后隐藏导航
        ui::ImageView *pHighlight = dynamic_cast<ui::ImageView *>(this->getChildByName( utility::toString("Image_Wheel_", j) ));
        pHighlight->setVisible(false);
    }
    //萧山麻将：打出财神（真身）喊"飘"；白板是财神替身——财神非白板时，打出白板按"它所代替的财神牌"报牌名
    if (OutCard.cbOutCardData == GameLogic::s_JokerCard) {
        UIHelper::playSound(utility::toString("raw/Mahjong/"
                                    , m_GameEngine->getPlayer(OutCard.cbOutCardUser)->getSexAsStr()
                                    , "/piao.mp3"));     //财飘语音
        return true;
    }
    uint8_t cbSoundCard = OutCard.cbOutCardData;
    if (cbSoundCard == WHITE_CARD && GameLogic::s_JokerCard != WHITE_CARD) {
        cbSoundCard = GameLogic::s_JokerCard;
    }
    UIHelper::playSound(utility::toString("raw/Mahjong/"
                                , m_GameEngine->getPlayer(OutCard.cbOutCardUser)->getSexAsStr()
                                , "/mjt"
                                , utility::toString(((cbSoundCard & MASK_COLOR) >> 4) + 1, "_", cbSoundCard & MASK_VALUE)
                                , ".mp3"));    //播放牌的声音
    return true;
}


/**
 * 操作提示
 * @param OperateNotify
 * @return
 */
bool GameLayer::onOperateNotifyEvent(CMD_S_OperateNotify OperateNotify) {
    return showOperateNotify(OperateNotify);
}

/**
 * 操作结果事件
 * @param OperateResult
 * @return
 */
bool GameLayer::onOperateResultEvent(CMD_S_OperateResult OperateResult) {
    //更新数据
    tagWeaveItem weaveItem;
    memset(&weaveItem, 0, sizeof(tagWeaveItem));
    switch (OperateResult.cbOperateCode) {
        case WIK_NULL: {
            break;
        }
        case WIK_P: {
            weaveItem.cbWeaveKind = WIK_P;
            weaveItem.cbCenterCard = OperateResult.cbOperateCard;
            weaveItem.cbPublicCard = TRUE;
            weaveItem.cbProvideUser = OperateResult.cbProvideUser;
            weaveItem.cbValid = TRUE;
            m_WeaveItemArray[OperateResult.cbOperateUser][m_cbWeaveItemCount[OperateResult.cbOperateUser]++] = weaveItem;
            if (OperateResult.cbOperateUser == m_MeChairID) {  //自己
                uint8_t cbReomveCard[] = {OperateResult.cbOperateCard, OperateResult.cbOperateCard};
                GameLogic::removeCard(m_cbCardIndex[OperateResult.cbOperateUser], cbReomveCard, sizeof(cbReomveCard));
            }
            break;
        }
        case WIK_G: {
            weaveItem.cbWeaveKind = WIK_G;
            weaveItem.cbCenterCard = OperateResult.cbOperateCard;
            uint8_t cbPublicCard = (OperateResult.cbOperateUser == OperateResult.cbProvideUser) ? FALSE : TRUE;
            int j = -1;
            for (int i = 0; i < m_cbWeaveItemCount[OperateResult.cbOperateUser]; i++) {
                tagWeaveItem tempWeaveItem = m_WeaveItemArray[OperateResult.cbOperateUser][i];
                if (tempWeaveItem.cbCenterCard == OperateResult.cbOperateCard) {   //之前已经存在
                    cbPublicCard = TRUE;
                    j = i;
                }
            }
            weaveItem.cbPublicCard = cbPublicCard;
            weaveItem.cbProvideUser = OperateResult.cbProvideUser;
            weaveItem.cbValid = TRUE;
            if (j == -1) {
                m_WeaveItemArray[OperateResult.cbOperateUser][m_cbWeaveItemCount[OperateResult.cbOperateUser]++] = weaveItem;
            } else {
                m_WeaveItemArray[OperateResult.cbOperateUser][j] = weaveItem;
            }
            if (OperateResult.cbOperateUser == m_MeChairID) {  //自己
                GameLogic::removeAllCard(m_cbCardIndex[OperateResult.cbOperateUser], OperateResult.cbOperateCard);
            }
            break;
        }
        case WIK_H: {
            break;
        }
        default:
            break;
    }

    //更新界面
    uint8_t cbViewID = m_GameEngine->switchViewChairID(OperateResult.cbOperateUser, m_MeChairID);
    switch (cbViewID) {
        case 0: {   //自己操作反馈
            m_pOperateNotifyGroup->removeAllChildren();
            m_pOperateNotifyGroup->setVisible(false);
            ui::ImageView *pHighlight = dynamic_cast<ui::ImageView *>(this->getChildByName( "Image_Wheel_0" ));
            pHighlight->setVisible(true);
            break;
        }
        default:
            break;
    }
    std::string strSex = m_GameEngine->getPlayer(OperateResult.cbOperateUser)->getSexAsStr();
    switch (OperateResult.cbOperateCode) {
        case WIK_NULL: {
            break;
        }
        case WIK_P: {
            UIHelper::playSound(utility::toString("raw/Mahjong/", strSex, "/peng.mp3"));
            if (cbViewID == 0) {                    //如果是自己，碰完需要出牌
                uint8_t bTempCardData[MAX_COUNT];   //手上的牌
                memset(bTempCardData, 0, sizeof(bTempCardData));
                GameLogic::switchToCardData(m_cbCardIndex[OperateResult.cbOperateUser], bTempCardData, MAX_COUNT);
                uint8_t cbWeaveItemCount = m_cbWeaveItemCount[OperateResult.cbOperateUser]; //组合数量
                CMD_S_SendCard SendCard;
                memset(&SendCard, 0, sizeof(CMD_S_SendCard));
                SendCard.cbCurrentUser = OperateResult.cbOperateUser;
                SendCard.cbCardData = bTempCardData[MAX_COUNT - (cbWeaveItemCount * 3) - 1];
                showSendCard(SendCard);    //模拟发送一张牌
            }
            //移除桌上的哪张牌
            m_cbDiscardCount[OperateResult.cbProvideUser]--;  //移除桌上的牌
            showAndUpdateDiscardCard();
            break;
        }
        case WIK_G: {
            UIHelper::playSound(utility::toString("raw/Mahjong/", strSex, "/gang.mp3"));
            if (OperateResult.cbProvideUser != OperateResult.cbOperateUser) {     //放的杠
                m_cbDiscardCount[OperateResult.cbProvideUser]--;  //移除桌上的牌
                showAndUpdateDiscardCard();
            } else {                                                            //移除出牌位置的牌
                ui::Layout *pRecvCardList = dynamic_cast<ui::Layout *>(UIHelper::seekNodeByName(m_PlayerPanel[0], utility::toString("RecvHandCard_0")));
                pRecvCardList->removeAllChildren();
            }
            break;
        }
        case WIK_H: { //胡牌动作放在游戏结束 信息中
            if (OperateResult.cbOperateUser == OperateResult.cbProvideUser) {  //自摸
                UIHelper::playSound(utility::toString("raw/Mahjong/", strSex, "/zimo.mp3"));
            } else {
                UIHelper::playSound(utility::toString("raw/Mahjong/", strSex, "/hu.mp3"));
            }
            break;
        }
        default:
            break;
    }
    showAndPlayOperateEffect(cbViewID, OperateResult.cbOperateCode, OperateResult.cbProvideUser == OperateResult.cbOperateUser);
    showAndUpdateHandCard();
    return true;
}

/**
 * 游戏结束事件
 * @param GameEnd
 * @return
 */
bool GameLayer::onGameEndEvent(CMD_S_GameEnd& GameEnd) {
    showAndUpdateUserScore(GameEnd.lGameScoreTable);    //更新分数
    for (uint8_t i = 0; i < GAME_PLAYER; i++) {                    //播放音效
        if (FvMask::HasAny(GameEnd.cbHuUser, _MASK_(i))) {
            std::string strSex = m_GameEngine->getPlayer(i)->getSexAsStr();
            uint8_t cbViewID = m_GameEngine->switchViewChairID(i, m_MeChairID);
            if (FvMask::HasAny(GameEnd.cbHuUser, _MASK_(GameEnd.cbProvideUser))) {  //自摸
                UIHelper::playSound(utility::toString("raw/Mahjong/", strSex, "/zimo.mp3"));
                showAndPlayOperateEffect(cbViewID, WIK_H, true); //播放自摸动画
            } else {
                UIHelper::playSound(utility::toString("raw/Mahjong/", strSex, "/hu.mp3"));
                showAndPlayOperateEffect(cbViewID, WIK_H, false); //播放自摸动画
            }
        }
    }
    Node *pTingNode = UIHelper::seekNodeByName(m_PlayerPanel[0], "ting_0");    //隐藏听牌
    pTingNode->setVisible(false);
    this->removeEffectNode("EffectNode");
    //清除桌面牌墙、翻出的财神牌与骰子
    Node *pWall = m_rootNode->getChildByName("CardWall");
    if (pWall) {
        pWall->removeFromParent();
    }
    m_pCardWall = NULL;
    Node *pJoker = m_rootNode->getChildByName("JokerCard");
    if (pJoker) {
        pJoker->removeFromParent();
    }
    removeDiceNodes();
    this->m_pOperateNotifyGroup->removeAllChildren();
    this->m_pOperateNotifyGroup->setVisible(false);
    //显示结算界面
    this->runAction(Sequence::createWithTwoActions(DelayTime::create(0.1f), CallFuncN::create([this, GameEnd](Node*) {
        auto ui = GameOverDlg::create();
        ui->setGameUI(this, GameEnd);
        DialogManager::shared()->showDialog(ui);
        const auto &aChildren = this->m_pOperateNotifyGroup->getChildren();
        log("m_pOperateNotifyGroup.getChildren=%d", (int)aChildren.size());
    })));
    return true;
}

void GameLayer::sendCardTimerUpdate(float) {
    m_iOutCardTimeOut = static_cast<uint8_t>((m_iOutCardTimeOut-- < 1) ? 0 : m_iOutCardTimeOut);
    ui::ImageView *pTimer1 = dynamic_cast<ui::ImageView *>(this->getChildByName( "Image_Timer_1" ));
    ui::ImageView *pTimer0 = dynamic_cast<ui::ImageView *>(this->getChildByName( "Image_Timer_0" ));
    int t = m_iOutCardTimeOut / 10;   //十位
    int g = m_iOutCardTimeOut % 10;   //个位
    pTimer1->loadTexture(utility::toString("res/GameLayer/timer_", g, ".png"));
    pTimer0->loadTexture(utility::toString("res/GameLayer/timer_", t, ".png"));
}


/**
 * 发牌显示
 * @param SendCard
 * @return
 */
bool GameLayer::showSendCard(CMD_S_SendCard SendCard) {
    m_pOperateNotifyGroup->removeAllChildren();
    m_pOperateNotifyGroup->setVisible(true);
    this->unschedule(schedule_selector(GameLayer::sendCardTimerUpdate));
    this->schedule(schedule_selector(GameLayer::sendCardTimerUpdate), 1.0f);    //出牌计时
    sendCardTimerUpdate(1.0f);//立即执行
    m_pTextCardNum->setString(utility::toString((int) m_cbLeftCardCount));
    uint8_t cbViewID = m_GameEngine->switchViewChairID(SendCard.cbCurrentUser, m_MeChairID);
    m_bOperate = false;
    switch (cbViewID) {
        case 0: {
            m_bOperate = true;   //允许选牌
            ui::Layout *pRecvCardList = dynamic_cast<ui::Layout *>(UIHelper::seekNodeByName(m_PlayerPanel[cbViewID], utility::toString("RecvHandCard_0")));
            pRecvCardList->removeAllChildren();
            ui::ImageView *pCard = UIHelper::createHandCardImageView(cbViewID, SendCard.cbCardData);
            pCard->setAnchorPoint(Vec2(0, 0));
            pCard->setPosition(Vec2(0, 0));
            pCard->setTouchEnabled(true);
            pCard->setTag(SendCard.cbCardData);
            pCard->setName(utility::toString("bt_card_", (int) SendCard.cbCardData));
            pCard->addTouchEventListener(CC_CALLBACK_2(GameLayer::onCardTouch, this));
            pRecvCardList->addChild(pCard);
            UIHelper::seekNodeByName(m_PlayerPanel[cbViewID], "ting_0")->setVisible(false); //拿到牌，隐藏听牌界面
            if (SendCard.cbActionMask != WIK_NULL) {//发的牌存在动作，模拟发送操作通知
                CMD_S_OperateNotify OperateNotify;
                memset(&OperateNotify, 0, sizeof(CMD_S_OperateNotify));
                OperateNotify.cbActionMask = SendCard.cbActionMask;
                OperateNotify.cbActionCard = SendCard.cbCardData;
                OperateNotify.cbGangCount = SendCard.cbGangCount;
                memcpy(OperateNotify.cbGangCard, SendCard.cbGangCard, sizeof(OperateNotify.cbGangCard));
                showOperateNotify(OperateNotify);
            }
            break;
        }
        case 1:
        case 2:
        case 3:
        default:
            m_bOperate = false;  //不允许操作
            break;
    }
    for (int i = 0; i < GAME_PLAYER; i++) {
        ui::ImageView *pRecvCard = dynamic_cast<ui::ImageView *>(UIHelper::seekNodeByName(m_PlayerPanel[cbViewID], utility::toString("RecvCard_", i)));
        if (pRecvCard) {
            pRecvCard->setVisible(cbViewID == i);
        }
        ui::ImageView *pHighlight = dynamic_cast<ui::ImageView *>(this->getChildByName( utility::toString("Image_Wheel_", i) ));
        if (pHighlight) {
            pHighlight->setVisible(cbViewID == i);
        }
    }
    return true;
}

/**
 * 牌墙槽位坐标（俯视图）
 * 四边各17墩：下(视角0)→左(视角1)→上(视角2)→右(视角3)按环序排列；
 * 每墩2张，墩顶那张向桌心方向错开一点，显出厚度
 * @param iSlot 环上槽位（0-135）
 * @return 槽位坐标
 */
Vec2 GameLayer::wallSlotPos(int iSlot) {
    int iSide = iSlot / 34;     //边（视角）
    int iTile = iSlot % 34;     //边内序号
    int iStack = iTile / 2;     //墩序号（沿环行进方向）
    Vec2 pos;
    switch (iSide) {
        case 0: pos = Vec2(830.0f - 24.0f * iStack, 152.0f); break;         //下边：右→左
        case 1: pos = Vec2(406.0f, 172.0f + 23.5f * iStack); break;         //左边：下→上
        case 2: pos = Vec2(446.0f + 24.0f * iStack, 582.0f); break;         //上边：左→右
        case 3: default: pos = Vec2(874.0f, 548.0f - 23.5f * iStack); break;//右边：上→下
    }
    if (iTile % 2 == 1) {   //墩顶的牌向桌心错开2.5px，显出两层
        switch (iSide) {
            case 0: pos.y += 2.5f; break;
            case 1: pos.x += 2.5f; break;
            case 2: pos.y -= 2.5f; break;
            case 3: default: pos.x -= 2.5f; break;
        }
    }
    return pos;
}

/**
 * 绘制桌面四边牌墙（俯视图，每家面前17墩）
 * 萧山麻将起牌规则：庄家掷两颗骰子，
 *   点数和 5/9 对本家(庄家)、2/6/10 对下家、3/7/11 对对家、4/8/12 对上家的牌墙；
 *   再从该牌墙尾部数「点数和」墩，从那一墩开始拿牌。
 * 发牌(13*4张)与翻财神(1张)已消耗的槽位不画，形成起牌缺口
 */
void GameLayer::showCardWall(uint8_t cbBankerChair, uint8_t cbDice1, uint8_t cbDice2) {
    Node *pOld = m_rootNode->getChildByName("CardWall");
    if (pOld) {
        pOld->removeFromParent();
    }
    m_pCardWall = Node::create();
    m_pCardWall->setName("CardWall");
    m_rootNode->addChild(m_pCardWall, 5);
    //骰子点数和定方位（相对庄家）
    uint8_t cbSum = static_cast<uint8_t>(cbDice1 + cbDice2);
    uint8_t cbSideChair = cbBankerChair;    //默认本家（5/9）
    switch (cbSum) {
        case 2: case 6: case 10: cbSideChair = static_cast<uint8_t>((cbBankerChair + 1) % GAME_PLAYER); break;  //下家
        case 3: case 7: case 11: cbSideChair = static_cast<uint8_t>((cbBankerChair + 2) % GAME_PLAYER); break;  //对家
        case 4: case 8: case 12: cbSideChair = static_cast<uint8_t>((cbBankerChair + 3) % GAME_PLAYER); break;  //上家
        default: break;
    }
    uint8_t cbSideView = m_GameEngine->switchViewChairID(cbSideChair, m_MeChairID);
    //起牌槽位：该墙从尾部数第 cbSum 墩开始拿（先拿墩顶那张）
    m_iWallFirstSlot = cbSideView * 34 + (17 - cbSum) * 2 + 1;
    //已消耗的槽位数（含发牌与翻财神）
    int iConsumed = MAX_REPERTORY - m_cbLeftCardCount;
    for (int p = 0; p < MAX_REPERTORY; p++) {
        int iDist = (m_iWallFirstSlot - p + MAX_REPERTORY) % MAX_REPERTORY;
        if (iDist < iConsumed) {
            continue;   //已被拿走的槽位不画
        }
        int iSide = p / 34;
        ui::ImageView *pBack = ui::ImageView::create(UIHelper::getBackCardImagePath(0, 0));
        pBack->setScale(0.28f);                     //牌背缩成俯视小牌 21x33
        if (iSide == 1 || iSide == 3) {
            pBack->setRotation(90.0f);              //左右两边横放
        }
        pBack->setPosition(wallSlotPos(p));
        pBack->setName(utility::toString("slot_", p));
        m_pCardWall->addChild(pBack);
    }
}

/**
 * 摸一张牌，牌墙起牌口少一张（最靠缺口的那张飞走）
 */
void GameLayer::popCardWallOne() {
    if (m_pCardWall == NULL || m_iWallFirstSlot < 0) {
        return;
    }
    int iConsumed = MAX_REPERTORY - m_cbLeftCardCount;     //含刚摸的这张
    int iSlot = ((m_iWallFirstSlot - (iConsumed - 1)) % MAX_REPERTORY + MAX_REPERTORY) % MAX_REPERTORY;
    Node *pTile = m_pCardWall->getChildByName(utility::toString("slot_", iSlot));
    if (pTile == NULL) {
        return;
    }
    //向上抽出并淡出，视觉上从牌墙里被摸走
    pTile->runAction(Sequence::create(
            Spawn::create(MoveBy::create(0.20f, Vec2(0.0f, 40.0f)), FadeOut::create(0.20f), nullptr),
            RemoveSelf::create(),
            nullptr));
}

/**
 * 财神牌翻出后正面朝上摆在牌墙旁边（起牌缺口结束处向桌心偏移）
 * @param cbCardData 翻出的那张财神牌
 */
void GameLayer::showJokerCard(uint8_t cbCardData) {
    Node *pOld = m_rootNode->getChildByName("JokerCard");
    if (pOld) {
        pOld->removeFromParent();
    }
    if (cbCardData == 0 || m_iWallFirstSlot < 0) {
        return;     //没有翻出财神
    }
    //翻财神那张牌的槽位 = 发完13*4张后的下一张
    int iJokerSlot = ((m_iWallFirstSlot - 52) % MAX_REPERTORY + MAX_REPERTORY) % MAX_REPERTORY;
    Vec2 pos = wallSlotPos(iJokerSlot);
    switch (iJokerSlot / 34) {  //向桌心偏移，摆在牌墙内侧
        case 0: pos += Vec2(0.0f, 44.0f); break;
        case 1: pos += Vec2(44.0f, 0.0f); break;
        case 2: pos += Vec2(0.0f, -44.0f); break;
        case 3: default: pos += Vec2(-44.0f, 0.0f); break;
    }
    Node *pJoker = Node::create();
    pJoker->setName("JokerCard");
    pJoker->setPosition(pos);
    m_rootNode->addChild(pJoker, 6);
    //财神牌正面
    ui::ImageView *pCard = UIHelper::createDiscardCardImageView(0, cbCardData);
    pCard->setScale(0.45f);
    pCard->setPosition(Vec2(0.0f, 0.0f));
    pJoker->addChild(pCard);
    //「财神」提示
    ui::Text *pLabel = ui::Text::create("财神", "res/DFYuanW7-GB2312.ttf", 16);
    pLabel->setTextColor(Color4B(255, 40, 40, 255));
    pLabel->setPosition(Vec2(0.0f, -38.0f));
    pJoker->addChild(pLabel);
}

/**
 * 构建一颗骰子（白底方片+点数层）
 */
Node *GameLayer::buildDieNode() {
    Node *pDie = Node::create();
    DrawNode *pBg = DrawNode::create();
    pBg->drawSolidRect(Vec2(-24.0f, -24.0f), Vec2(24.0f, 24.0f), Color4F(0.96f, 0.96f, 0.94f, 1.0f));
    pBg->drawRect(Vec2(-24.0f, -24.0f), Vec2(24.0f, 24.0f), Color4F(0.45f, 0.45f, 0.45f, 1.0f));
    pDie->addChild(pBg);
    Node *pFace = Node::create();
    pFace->setName("Face");
    pDie->addChild(pFace);
    return pDie;
}

/**
 * 设置骰子点数（标准骰面布局）
 */
void GameLayer::setDieFace(Node *pDie, uint8_t cbValue) {
    Node *pFace = pDie->getChildByName("Face");
    if (pFace == NULL) {
        return;
    }
    pFace->removeAllChildren();
    if (cbValue < 1) cbValue = 1;
    if (cbValue > 6) cbValue = 6;
    DrawNode *pDots = DrawNode::create();
    const float r = 4.5f;
    const Color4F dot(0.13f, 0.13f, 0.13f, 1.0f);
    const Vec2 c(0.0f, 0.0f), tl(-12.0f, 12.0f), br(12.0f, -12.0f), tr(12.0f, 12.0f), bl(-12.0f, -12.0f), ml(-12.0f, 0.0f), mr(12.0f, 0.0f);
    switch (cbValue) {
        case 1: pDots->drawDot(c, r + 1.5f, dot); break;
        case 2: pDots->drawDot(tl, r, dot); pDots->drawDot(br, r, dot); break;
        case 3: pDots->drawDot(tl, r, dot); pDots->drawDot(c, r, dot); pDots->drawDot(br, r, dot); break;
        case 4: pDots->drawDot(tl, r, dot); pDots->drawDot(tr, r, dot); pDots->drawDot(bl, r, dot); pDots->drawDot(br, r, dot); break;
        case 5: pDots->drawDot(tl, r, dot); pDots->drawDot(tr, r, dot); pDots->drawDot(c, r, dot); pDots->drawDot(bl, r, dot); pDots->drawDot(br, r, dot); break;
        case 6: default: pDots->drawDot(tl, r, dot); pDots->drawDot(ml, r, dot); pDots->drawDot(bl, r, dot); pDots->drawDot(tr, r, dot); pDots->drawDot(mr, r, dot); pDots->drawDot(br, r, dot); break;
    }
    pFace->addChild(pDots);
}

/**
 * 清理骰子相关节点与定时任务
 */
void GameLayer::removeDiceNodes() {
    this->unschedule("DiceRollAnim");
    this->unschedule("DiceAnimDone");
    this->unschedule("AiAutoDice");
    Node *pDice = m_rootNode->getChildByName("DiceGroup");
    if (pDice) {
        pDice->removeFromParent();
    }
    Node *pBtn = m_rootNode->getChildByName("DiceRollBtn");
    if (pBtn) {
        pBtn->removeFromParent();
    }
}

/**
 * 播放庄家掷骰动画：两颗骰子快速滚动后落定，随后开始发牌
 */
void GameLayer::playDiceAnimation(CMD_S_GameStart GameStart) {
    removeDiceNodes();
    Node *pGroup = Node::create();
    pGroup->setName("DiceGroup");
    pGroup->setPosition(Vec2(640.0f, 455.0f));  //牌桌中央（倒计时轮盘下方）
    m_rootNode->addChild(pGroup, 20);
    Node *pDie1 = buildDieNode();
    pDie1->setPosition(Vec2(-34.0f, 0.0f));
    pGroup->addChild(pDie1);
    Node *pDie2 = buildDieNode();
    pDie2->setPosition(Vec2(34.0f, 0.0f));
    pGroup->addChild(pDie2);
    //提示：点数和决定从哪家牌墙第几墩起牌
    uint8_t cbSum = static_cast<uint8_t>(GameStart.cbDice1 + GameStart.cbDice2);
    const char *strSide = "本家";
    switch (cbSum) {
        case 2: case 6: case 10: strSide = "下家"; break;
        case 3: case 7: case 11: strSide = "对家"; break;
        case 4: case 8: case 12: strSide = "上家"; break;
        default: break;
    }
    ui::Text *pText = ui::Text::create(utility::toString((int) GameStart.cbDice1, "+", (int) GameStart.cbDice2,
            " 从", strSide, "牌墙第", (int) cbSum, "墩起牌"), "res/DFYuanW7-GB2312.ttf", 18);
    pText->setTextColor(Color4B(255, 235, 130, 255));
    pText->setPosition(Vec2(0.0f, -46.0f));
    pGroup->addChild(pText);
    //滚动动画：快速变换点数后落定
    auto pStep = std::make_shared<int>(0);
    this->schedule([this, pGroup, pDie1, pDie2, GameStart, pStep](float) {
        if (++(*pStep) >= 14) {
            this->unschedule("DiceRollAnim");
            setDieFace(pDie1, GameStart.cbDice1);
            setDieFace(pDie2, GameStart.cbDice2);
            pDie1->runAction(Sequence::create(ScaleTo::create(0.08f, 1.25f), ScaleTo::create(0.10f, 1.0f), nullptr));
            pDie2->runAction(Sequence::create(ScaleTo::create(0.08f, 1.25f), ScaleTo::create(0.10f, 1.0f), nullptr));
            this->scheduleOnce([this, pGroup, GameStart](float) {
                pGroup->runAction(Sequence::create(FadeOut::create(0.30f), RemoveSelf::create(), nullptr));
                finishGameStartUI(GameStart);   //骰子落定后开始发牌
            }, 0.60f, "DiceAnimDone");
        } else {
            setDieFace(pDie1, static_cast<uint8_t>(rand() % 6 + 1));
            setDieFace(pDie2, static_cast<uint8_t>(rand() % 6 + 1));
        }
    }, 0.07f, "DiceRollAnim");
}

/**
 * 人是庄家：显示「掷骰子」按钮，点击后播放掷骰动画
 */
void GameLayer::showDiceRollButton(CMD_S_GameStart GameStart) {
    removeDiceNodes();
    ui::Button *pBtn = ui::Button::create("res/GameLayer/btn_action1.png");
    pBtn->setName("DiceRollBtn");
    pBtn->setTitleText("掷骰子");
    pBtn->setTitleFontSize(26);
    pBtn->setTitleFontName("res/DFYuanW7-GB2312.ttf");
    pBtn->setScale(0.9f);
    pBtn->setPosition(Vec2(640.0f, 475.0f));
    pBtn->addClickEventListener([this, GameStart](Ref *) {
        playDiceAnimation(GameStart);   //点击掷骰（动画开始时移除按钮）
    });
    m_rootNode->addChild(pBtn, 20);
}

/**
 * 视图座位名（你/下家/对家/上家）
 */
static const char *getDiceViewName(uint8_t cbViewID) {
    static const char *kViewName[GAME_PLAYER] = {"你", "下家", "对家", "上家"};
    return kViewName[cbViewID % GAME_PLAYER];
}

/**
 * 首局：每人掷骰定庄流程（依次四家掷两颗，点数最大者为庄；AI自动掷，真人点按钮）
 */
void GameLayer::showBankerDiceFlow(CMD_S_GameStart GameStart) {
    m_iBankerDiceStep = 0;
    Node *pGroup = Node::create();
    pGroup->setName("DiceGroup");
    pGroup->setPosition(Vec2(640.0f, 430.0f));
    m_rootNode->addChild(pGroup, 20);
    //标题
    ui::Text *pTitle = ui::Text::create("掷骰定庄", "res/DFYuanW7-GB2312.ttf", 22);
    pTitle->setName("DiceTitle");
    pTitle->setTextColor(Color4B(255, 235, 130, 255));
    pTitle->setPosition(Vec2(0.0f, 70.0f));
    pGroup->addChild(pTitle);
    //两颗骰子（滚动时放大、落定回缩）
    Node *pDie1 = buildDieNode();
    pDie1->setName("Die1");
    pDie1->setPosition(Vec2(-34.0f, 18.0f));
    pDie1->setScale(0.8f);
    pGroup->addChild(pDie1);
    Node *pDie2 = buildDieNode();
    pDie2->setName("Die2");
    pDie2->setPosition(Vec2(34.0f, 18.0f));
    pDie2->setScale(0.8f);
    pGroup->addChild(pDie2);
    //各家结果行（名字按视图位置，未掷显示灰色「待掷」）
    for (uint8_t i = 0; i < GAME_PLAYER; i++) {
        uint8_t cbViewID = m_GameEngine->switchViewChairID(i, m_MeChairID);
        ui::Text *pText = ui::Text::create(utility::toString(getDiceViewName(cbViewID), "：待掷"),
                                           "res/DFYuanW7-GB2312.ttf", 18);
        pText->setName(utility::toString("BankerDice_", (int) i));
        pText->setTextColor(Color4B(180, 180, 180, 255));
        pText->setPosition(Vec2(0.0f, -50.0f - cbViewID * 26.0f));
        pGroup->addChild(pText);
    }
    playBankerDiceStep(GameStart);
}

/**
 * 定庄流程：轮到第 m_iBankerDiceStep 家掷骰（真人是自己则显示按钮，AI 自动）
 */
void GameLayer::playBankerDiceStep(CMD_S_GameStart GameStart) {
    if (m_iBankerDiceStep >= GAME_PLAYER) {    //四家都比完：宣布庄家，随后庄家再掷定位骰
        Node *pGroup = m_rootNode->getChildByName("DiceGroup");
        if (pGroup) {
            ui::Text *pTitle = dynamic_cast<ui::Text *>(pGroup->getChildByName("DiceTitle"));
            if (pTitle) {
                uint8_t cbBankerView = m_GameEngine->switchViewChairID(GameStart.cbBankerUser, m_MeChairID);
                pTitle->setString(utility::toString(getDiceViewName(cbBankerView), " ",
                        (int) GameStart.cbBankerDice[GameStart.cbBankerUser][0], "+",
                        (int) GameStart.cbBankerDice[GameStart.cbBankerUser][1], " 点最大，成为庄家"));
                pTitle->setTextColor(Color4B(120, 255, 120, 255));
            }
        }
        //比完大小：庄家再掷一次骰子选定起牌位置，之后才开始发牌
        this->scheduleOnce([this, GameStart](float) {
            playBankerPositionDice(GameStart);
        }, 1.20f, "DiceAnimDone");
        return;
    }
    uint8_t cbChair = static_cast<uint8_t>(m_iBankerDiceStep);
    //真人判定：视图0（自己）为准，椅子相等兜底——避免 m_MeChairID 时序异常导致误判成AI自动掷
    uint8_t cbChairView = m_GameEngine->switchViewChairID(cbChair, m_MeChairID);
    bool bMeTurn = (cbChairView == 0 || cbChair == m_MeChairID);
    cocos2d::log("定庄掷骰：第%d家 椅子%d 视图%d 我椅子%d 是否轮到我=%d",
                 m_iBankerDiceStep, cbChair, cbChairView, m_MeChairID, bMeTurn ? 1 : 0);
    if (bMeTurn) {
        //真人掷骰：显示按钮
        Node *pOld = m_rootNode->getChildByName("DiceRollBtn");
        if (pOld) { pOld->removeFromParent(); }
        ui::Button *pBtn = ui::Button::create("res/GameLayer/btn_action1.png");
        pBtn->setName("DiceRollBtn");
        pBtn->setTitleText("掷骰子");
        pBtn->setTitleFontSize(26);
        pBtn->setTitleFontName("res/DFYuanW7-GB2312.ttf");
        pBtn->setScale(0.9f);
        pBtn->setPosition(Vec2(640.0f, 240.0f));    //骰盘下方
        pBtn->addClickEventListener([this, cbChair, GameStart](Ref *) {
            Node *pSelf = m_rootNode->getChildByName("DiceRollBtn");
            if (pSelf) { pSelf->removeFromParent(); }
            rollBankerDiceAnim(cbChair, GameStart);
        });
        m_rootNode->addChild(pBtn, 21);
    } else {
        //AI 掷骰：自动
        this->scheduleOnce([this, cbChair, GameStart](float) {
            rollBankerDiceAnim(cbChair, GameStart);
        }, 0.45f, "AiAutoDice");
    }
}

/**
 * 定庄流程：滚动一家掷骰动画，落定后记录点数并轮到下一家
 */
void GameLayer::rollBankerDiceAnim(uint8_t cbChair, CMD_S_GameStart GameStart) {
    Node *pGroup = m_rootNode->getChildByName("DiceGroup");
    if (pGroup == NULL) { return; }
    Node *pDie1 = pGroup->getChildByName("Die1");
    Node *pDie2 = pGroup->getChildByName("Die2");
    if (pDie1 == NULL || pDie2 == NULL) { return; }
    uint8_t cbD1 = GameStart.cbBankerDice[cbChair][0];
    uint8_t cbD2 = GameStart.cbBankerDice[cbChair][1];
    auto pStep = std::make_shared<int>(0);
    this->schedule([this, pGroup, pDie1, pDie2, cbChair, cbD1, cbD2, GameStart, pStep](float) {
        if (++(*pStep) >= 14) {
            this->unschedule("DiceRollAnim");
            setDieFace(pDie1, cbD1);
            setDieFace(pDie2, cbD2);
            pDie1->runAction(Sequence::create(ScaleTo::create(0.08f, 1.0f), ScaleTo::create(0.10f, 0.8f), nullptr));
            pDie2->runAction(Sequence::create(ScaleTo::create(0.08f, 1.0f), ScaleTo::create(0.10f, 0.8f), nullptr));
            //更新该家结果行
            uint8_t cbViewID = m_GameEngine->switchViewChairID(cbChair, m_MeChairID);
            ui::Text *pText = dynamic_cast<ui::Text *>(pGroup->getChildByName(utility::toString("BankerDice_", (int) cbChair)));
            if (pText) {
                pText->setString(utility::toString(getDiceViewName(cbViewID), "：",
                        (int) cbD1, "+", (int) cbD2, "＝", (int) (cbD1 + cbD2)));
                pText->setTextColor(Color4B(255, 255, 255, 255));
            }
            //轮到下一家
            m_iBankerDiceStep++;
            this->scheduleOnce([this, GameStart](float) {
                playBankerDiceStep(GameStart);
            }, 0.35f, "AiAutoDice");
        } else {
            setDieFace(pDie1, static_cast<uint8_t>(rand() % 6 + 1));
            setDieFace(pDie2, static_cast<uint8_t>(rand() % 6 + 1));
        }
    }, 0.07f, "DiceRollAnim");
}

/**
 * 掷骰结束：亮手牌、翻财神、通知引擎发第一张牌
 */
void GameLayer::finishGameStartUI(CMD_S_GameStart GameStart) {
    if (m_bDiceConfirmed) {
        return;
    }
    m_bDiceConfirmed = true;
    //定位骰落定后才开始发牌：剩余牌数与牌墙缺口此时才呈现
    m_cbLeftCardCount = GameStart.cbLeftCardCount;
    m_pTextCardNum->setString(utility::toString((int) m_cbLeftCardCount));
    showCardWall(GameStart.cbBankerUser, GameStart.cbDice1, GameStart.cbDice2);    //重绘牌墙（呈现发牌缺口）
    showAndUpdateHandCard();
    showAndUpdateDiscardCard();
    showJokerCard(GameStart.cbJokerCard);   //财神翻出摆在牌墙旁
    m_GameEngine->onUserDiceConfirm();      //开始发牌（庄家摸第一张）
}

/**
 * 首局定庄后：庄家再掷一次骰子选定起牌位置（真人是庄家点按钮，AI庄家自动）
 */
void GameLayer::playBankerPositionDice(CMD_S_GameStart GameStart) {
    uint8_t cbBankerView = m_GameEngine->switchViewChairID(GameStart.cbBankerUser, m_MeChairID);
    bool bMeBanker = (cbBankerView == 0 || GameStart.cbBankerUser == m_MeChairID);
    cocos2d::log("定位骰阶段：庄家椅子%d 视图%d 我椅子%d 玩家坐庄=%d",
                 GameStart.cbBankerUser, cbBankerView, m_MeChairID, bMeBanker ? 1 : 0);
    Node *pGroup = m_rootNode->getChildByName("DiceGroup");
    if (pGroup) {
        ui::Text *pTitle = dynamic_cast<ui::Text *>(pGroup->getChildByName("DiceTitle"));
        if (pTitle) {
            pTitle->setString(utility::toString(getDiceViewName(cbBankerView), "掷骰定位"));
            pTitle->setTextColor(Color4B(255, 235, 130, 255));
        }
    }
    if (bMeBanker) {
        //真人是庄家：显示按钮
        Node *pOld = m_rootNode->getChildByName("DiceRollBtn");
        if (pOld) { pOld->removeFromParent(); }
        ui::Button *pBtn = ui::Button::create("res/GameLayer/btn_action1.png");
        pBtn->setName("DiceRollBtn");
        pBtn->setTitleText("掷骰子");
        pBtn->setTitleFontSize(26);
        pBtn->setTitleFontName("res/DFYuanW7-GB2312.ttf");
        pBtn->setScale(0.9f);
        pBtn->setPosition(Vec2(640.0f, 240.0f));    //骰盘下方
        pBtn->addClickEventListener([this, GameStart](Ref *) {
            Node *pSelf = m_rootNode->getChildByName("DiceRollBtn");
            if (pSelf) { pSelf->removeFromParent(); }
            rollPositionDiceAnim(GameStart);
        });
        m_rootNode->addChild(pBtn, 21);
    } else {
        //AI 庄家：自动掷定位骰
        this->scheduleOnce([this, GameStart](float) {
            rollPositionDiceAnim(GameStart);
        }, 0.45f, "AiAutoDice");
    }
}

/**
 * 定位骰动画：两颗骰子滚动落定，显示起牌位置（从哪家牌墙第几墩起牌），随后开始发牌
 */
void GameLayer::rollPositionDiceAnim(CMD_S_GameStart GameStart) {
    Node *pGroup = m_rootNode->getChildByName("DiceGroup");
    if (pGroup == NULL) { return; }
    Node *pDie1 = pGroup->getChildByName("Die1");
    Node *pDie2 = pGroup->getChildByName("Die2");
    if (pDie1 == NULL || pDie2 == NULL) { return; }
    uint8_t cbD1 = GameStart.cbDice1;   //定位骰（庄家比完大小后的第二次掷骰）
    uint8_t cbD2 = GameStart.cbDice2;
    auto pStep = std::make_shared<int>(0);
    this->schedule([this, pGroup, pDie1, pDie2, cbD1, cbD2, GameStart, pStep](float) {
        if (++(*pStep) >= 14) {
            this->unschedule("DiceRollAnim");
            setDieFace(pDie1, cbD1);
            setDieFace(pDie2, cbD2);
            pDie1->runAction(Sequence::create(ScaleTo::create(0.08f, 1.0f), ScaleTo::create(0.10f, 0.8f), nullptr));
            pDie2->runAction(Sequence::create(ScaleTo::create(0.08f, 1.0f), ScaleTo::create(0.10f, 0.8f), nullptr));
            //显示起牌位置（点数和定方位与墩数）
            uint8_t cbSum = static_cast<uint8_t>(cbD1 + cbD2);
            const char *strSide = "本家";
            switch (cbSum) {
                case 2: case 6: case 10: strSide = "下家"; break;
                case 3: case 7: case 11: strSide = "对家"; break;
                case 4: case 8: case 12: strSide = "上家"; break;
                default: break;
            }
            ui::Text *pTitle = dynamic_cast<ui::Text *>(pGroup->getChildByName("DiceTitle"));
            if (pTitle) {
                pTitle->setString(utility::toString((int) cbD1, "+", (int) cbD2,
                        " 从", strSide, "牌墙第", (int) cbSum, "墩起牌"));
                pTitle->setTextColor(Color4B(255, 235, 130, 255));
            }
            this->scheduleOnce([this, pGroup, GameStart](float) {
                if (pGroup) {
                    pGroup->runAction(Sequence::create(FadeOut::create(0.30f), RemoveSelf::create(), nullptr));
                }
                finishGameStartUI(GameStart);   //定位落定，开始发牌
            }, 1.00f, "DiceAnimDone");
        } else {
            setDieFace(pDie1, static_cast<uint8_t>(rand() % 6 + 1));
            setDieFace(pDie2, static_cast<uint8_t>(rand() % 6 + 1));
        }
    }, 0.07f, "DiceRollAnim");
}

/**
 * 庄家头像角标：首庄显示「庄」，连庄显示「N连庄」
 */
void GameLayer::showBankerStreakBadge() {
    for (uint8_t i = 0; i < GAME_PLAYER; i++) {   //清掉旧角标
        Node *pOld = m_FaceFrame[i]->getChildByName("BankerStreakBadge");
        if (pOld) { pOld->removeFromParent(); }
    }
    if (m_cbBankerChair == INVALID_CHAIR) { return; }
    uint8_t cbViewID = m_GameEngine->switchViewChairID(m_cbBankerChair, m_MeChairID);
    uint8_t cbStreak = m_GameEngine->getBankerStreak();
    if (cbStreak == 0) { cbStreak = 1; }
    ui::Text *pBadge = ui::Text::create(cbStreak >= 2 ? utility::toString((int) cbStreak, "连庄") : "庄",
                                        "res/DFYuanW7-GB2312.ttf", 18);
    if (pBadge == nullptr) { return; }
    pBadge->setName("BankerStreakBadge");
    pBadge->setTextColor(Color4B(255, 60, 60, 255));
    pBadge->enableOutline(Color4B(255, 255, 255, 200), 2);
    //放在庄家头像正上方
    ui::ImageView *pImageHeader = dynamic_cast<ui::ImageView *>(UIHelper::seekNodeByName(m_FaceFrame[cbViewID], "Image_Header"));
    Vec2 pos = (pImageHeader != nullptr)
               ? Vec2(pImageHeader->getPositionX(),
                      pImageHeader->getPositionY() + pImageHeader->getContentSize().height * 0.5f + 12.0f)
               : Vec2(m_FaceFrame[cbViewID]->getContentSize().width * 0.5f, 0.0f);
    pBadge->setPosition(pos);
    m_FaceFrame[cbViewID]->addChild(pBadge, 20);
}


bool GameLayer::showTingResult(const uint8_t *cbCardIndex, tagWeaveItem *WeaveItem, uint8_t cbWeaveCount) {
    tagTingResult tingResult;
    memset(&tingResult, 0, sizeof(tagTingResult));
    Node *pTingNode = UIHelper::seekNodeByName(m_PlayerPanel[m_MeChairID], "ting_0");
    if (GameLogic::analyseTingCardResult(cbCardIndex, WeaveItem, cbWeaveCount, tingResult)) {  //听牌
        pTingNode->setVisible(true);
        Node *pTingCard = UIHelper::seekNodeByName(m_PlayerPanel[m_MeChairID], "ting_card");
        pTingCard->removeAllChildren();
        for (int i = 0; i < tingResult.cbTingCount; i++) {  //循环听的牌
            ui::ImageView *pTingCardImage = UIHelper::createDiscardCardImageView(0, tingResult.cbTingCard[i]);
            pTingCardImage->setAnchorPoint(Vec2(0, 0));
            pTingCardImage->setPosition(Vec2(76 * i + ((76.0f * 3) - (76.0f * tingResult.cbTingCount)) / 2.0f, 0));
            pTingCard->addChild(pTingCardImage);
        }
    } else {
        pTingNode->setVisible(false);
    }
    return true;
}

bool GameLayer::showAndUpdateUserScore(const int64_t lGameScoreTable[GAME_PLAYER]) {
    for (uint8_t i = 0; i < GAME_PLAYER; i++) {
        uint8_t cbViewID = m_GameEngine->switchViewChairID(i, m_MeChairID);
        ui::Text *pScore = dynamic_cast<ui::Text *>(UIHelper::seekNodeByName(m_FaceFrame[cbViewID], "Text_Score"));  //CSB中分数节点名为Text_Score
        if (pScore) {
            pScore->setString(utility::toString(lGameScoreTable[i]));
        }
    }
    return true;
}

/**
 * 展示操作层
 * @param OperateNotify
 * @return
 */
bool GameLayer::showOperateNotify(CMD_S_OperateNotify OperateNotify) {
    if (OperateNotify.cbActionMask == WIK_NULL) {
        return true; //无动作
    }
    m_pOperateNotifyGroup->setVisible(true);
    m_pOperateNotifyGroup->removeAllChildren();
    auto loadUI = [this](const char* resPath, float x, float y, uint8_t tag) {
        Node *pNode = CSLoader::createNode(resPath);
        pNode->setPosition(Vec2(x, y));
        m_pOperateNotifyGroup->addChild(pNode);
        ui::Button *pBtn = dynamic_cast<ui::Button *>(pNode->getChildren().at(0));
        pBtn->setTag(tag);
        pBtn->addTouchEventListener([this, pBtn](Ref *ref, ui::Widget::TouchEventType eventType) {
            ui::Button *pRef = dynamic_cast<ui::Button *>(ref);
            if (pRef != NULL && pRef == pBtn) {
                std::string btnName = pRef->getName();
                int iTag = pRef->getTag();
                if (eventType == ui::Widget::TouchEventType::ENDED) {
                    CMD_C_OperateCard OperateCard;
                    memset(&OperateCard, 0, sizeof(CMD_C_OperateCard));
                    OperateCard.cbOperateCard = static_cast<uint8_t>(iTag);
                    OperateCard.cbOperateUser = m_MeChairID;
                    if (btnName == "btn_hu") {
                        OperateCard.cbOperateCode = WIK_H;
                    } else if (btnName == "btn_gang") {
                        OperateCard.cbOperateCode = WIK_G;
                    } else if (btnName == "btn_peng") {
                        OperateCard.cbOperateCode = WIK_P;
                    } else if (btnName == "btn_guo") {
                        OperateCard.cbOperateCode = WIK_NULL;
                    }
                    m_GameEngine->onUserOperateCard(OperateCard);
                    m_pOperateNotifyGroup->removeAllChildren();
                    m_pOperateNotifyGroup->setVisible(false);
                }
            }
        });
        return pBtn;
    };
    float x = 500.0f;
    float y = 65.0f;
    if ((OperateNotify.cbActionMask & WIK_H) == WIK_H) {
        loadUI("res/BtnHu.csb", x, y, OperateNotify.cbActionCard);
        x -= 160.0f;
    }
    if ((OperateNotify.cbActionMask & WIK_G) == WIK_G) {
        for (int i = 0; i < OperateNotify.cbGangCount; i++) {
            ui::Button *pGangBtn = loadUI("res/BtnGang.csb", x, y + (i * 120), OperateNotify.cbGangCard[i]);
            ui::ImageView *pImgGangCard = dynamic_cast<ui::ImageView *>(UIHelper::seekNodeByName(pGangBtn, "ImgGangCard"));
            pImgGangCard->loadTexture(UIHelper::getDiscardCardImagePath(0, OperateNotify.cbGangCard[i]));
            pImgGangCard->setVisible(OperateNotify.cbGangCount > 1);
            x -= 160.0f;
        }
    }
    if ((OperateNotify.cbActionMask & WIK_P) == WIK_P) {
        loadUI("res/BtnPeng.csb", x, y, OperateNotify.cbActionCard);
        x -= 160.0f;
    }
    loadUI("res/BtnGuo.csb", x, y, OperateNotify.cbActionCard);
    return true;
}

/**
 * 显示手上的牌
 * @return
 */
bool GameLayer::showAndUpdateHandCard() {
    for (uint8_t i = 0; i < m_GameEngine->getPlayerCount(); i++) {
        uint8_t viewChairID = m_GameEngine->switchViewChairID(i, m_MeChairID);
        uint8_t bCardData[MAX_COUNT];  //手上的牌
        memset(bCardData, 0, sizeof(bCardData));
        //萧山麻将：白板是财神的替身——显示时白板排在所代替的财神牌位置，真财神排到白板原位置
        GameLogic::switchToDisplayCardData(m_cbCardIndex[i], bCardData, MAX_COUNT);
        uint8_t cbWeaveItemCount = m_cbWeaveItemCount[i]; //组合数量
        tagWeaveItem WeaveItemArray[MAX_WEAVE];         //组合
        memcpy(WeaveItemArray, m_WeaveItemArray[i], sizeof(WeaveItemArray));
        switch (viewChairID) {
            case 0: {
                ui::Layout *pHandCard0 = dynamic_cast<ui::Layout *>(UIHelper::seekNodeByName(m_PlayerPanel[viewChairID], "HandCard_0"));
                ui::Layout *pComb0 = dynamic_cast<ui::Layout *>(UIHelper::seekNodeByName(m_PlayerPanel[viewChairID], "comb_0"));  //组合
                pHandCard0->removeAllChildren();
                pComb0->removeAllChildren();
                int x = 0;  //宽起始位置
                for (uint8_t j = 0; j < cbWeaveItemCount; j++) {
                    tagWeaveItem weaveItem = WeaveItemArray[j];
                    Node *pWeaveNode = NULL;
                    if (weaveItem.cbWeaveKind == WIK_G) {
                        pWeaveNode = CSLoader::createNode("res/Gang0.csb");
                    }
                    if (weaveItem.cbWeaveKind == WIK_P) {
                        pWeaveNode = CSLoader::createNode("res/Peng0.csb");
                    }
                    assert(pWeaveNode != NULL);
                    pWeaveNode->setPosition(Vec2(x, 0));
                    const std::string &strImagePath = UIHelper::getDiscardCardImagePath(0, weaveItem.cbCenterCard);
                    const std::string &strBackImagePath = UIHelper::getBackCardImagePath(0, weaveItem.cbCenterCard);
                    ui::ImageView *pImageRight = dynamic_cast<ui::ImageView *>(UIHelper::seekNodeByName(pWeaveNode, "Image_right"));
                    ui::ImageView *pImageLeft = dynamic_cast<ui::ImageView *>(UIHelper::seekNodeByName(pWeaveNode, "Image_left"));
                    ui::ImageView *pImageCenter = dynamic_cast<ui::ImageView *>(UIHelper::seekNodeByName(pWeaveNode, "Image_center"));
                    pImageRight->loadTexture(strImagePath);
                    pImageCenter->loadTexture(strImagePath);
                    pImageLeft->loadTexture(strImagePath);
                    switch (m_GameEngine->switchViewChairID(weaveItem.cbProvideUser, m_MeChairID)) {
                        case 0: {
                            if (weaveItem.cbPublicCard == FALSE) {    //暗杠
                                pImageRight->loadTexture(strBackImagePath);
                                pImageCenter->loadTexture(strImagePath);
                                pImageLeft->loadTexture(strBackImagePath);
                            }
                            break;
                        }
                        case 1: {
                            pImageLeft->loadTexture(strBackImagePath);
                            break;
                        }
                        case 2: {
                            pImageCenter->loadTexture(strBackImagePath);
                            break;
                        }
                        case 3: {
                            pImageRight->loadTexture(strBackImagePath);
                            break;
                        }
                        default:
                            break;
                    }
                    pComb0->addChild(pWeaveNode);
                    x += 228;
                }
                for (uint8_t j = 0; j < MAX_COUNT - 1 - (3 * cbWeaveItemCount); j++) {
                    ui::ImageView *pCard = UIHelper::createHandCardImageView(viewChairID, bCardData[j]);
                    pCard->setAnchorPoint(Vec2(0, 0));
                    pCard->setPosition(Vec2(x, 0));
                    pCard->setTouchEnabled(true);
                    pCard->setTag(bCardData[j]);
                    pCard->setName(utility::toString("bt_card_", (int) bCardData[j]));
                    pCard->addTouchEventListener(CC_CALLBACK_2(GameLayer::onCardTouch, this));
                    pHandCard0->addChild(pCard);
                    x += 76;
                }
                break;
            }
            case 1: {
                ui::Layout *pHandCard1 = dynamic_cast<ui::Layout *>(UIHelper::seekNodeByName(m_PlayerPanel[viewChairID], "HandCard_1"));
                ui::Layout *pComb1 = dynamic_cast<ui::Layout *>(UIHelper::seekNodeByName(m_PlayerPanel[viewChairID], "comb_1"));  //组合
                pHandCard1->removeAllChildren();
                pComb1->removeAllChildren();
                int y = 800;  //高起始位置
                for (uint8_t j = 0; j < cbWeaveItemCount; j++) {
                    y -= 160;
                    tagWeaveItem weaveItem = WeaveItemArray[j];
                    Node *pWeaveNode = NULL;
                    if (weaveItem.cbWeaveKind == WIK_G) {
                        pWeaveNode = CSLoader::createNode("res/Gang1.csb");
                    }
                    if (weaveItem.cbWeaveKind == WIK_P) {
                        pWeaveNode = CSLoader::createNode("res/Peng1.csb");
                    }
                    assert(pWeaveNode != NULL);
                    pWeaveNode->setPosition(Vec2(0, y + 20));
                    const std::string &strImagePath = UIHelper::getDiscardCardImagePath(1, weaveItem.cbCenterCard);
                    const std::string &strBackImagePath = UIHelper::getBackCardImagePath(1, weaveItem.cbCenterCard);
                    ui::ImageView *pImageRight = dynamic_cast<ui::ImageView *>(UIHelper::seekNodeByName(pWeaveNode, "Image_right"));
                    ui::ImageView *pImageLeft = dynamic_cast<ui::ImageView *>(UIHelper::seekNodeByName(pWeaveNode, "Image_left"));
                    ui::ImageView *pImageCenter = dynamic_cast<ui::ImageView *>(UIHelper::seekNodeByName(pWeaveNode, "Image_center"));
                    pImageRight->loadTexture(strImagePath);
                    pImageCenter->loadTexture(strImagePath);
                    pImageLeft->loadTexture(strImagePath);
                    switch (m_GameEngine->switchViewChairID(weaveItem.cbProvideUser, m_MeChairID)) {
                        case 0: {
                            pImageRight->loadTexture(strBackImagePath);
                            break;
                        }
                        case 1: {
                            if (weaveItem.cbPublicCard == FALSE) {    //暗杠
                                pImageRight->loadTexture(strBackImagePath);
                                pImageCenter->loadTexture(strBackImagePath);
                                pImageLeft->loadTexture(strBackImagePath);
                            }
                            break;
                        }
                        case 2: {
                            pImageLeft->loadTexture(strBackImagePath);
                            break;
                        }
                        case 3: {
                            pImageCenter->loadTexture(strBackImagePath);
                            break;
                        }
                        default:
                            break;
                    }
                    pComb1->addChild(pWeaveNode);
                }
                for (uint8_t j = 0; j < MAX_COUNT - 1 - (3 * cbWeaveItemCount); j++) {
                    y -= 60;
                    ui::ImageView *pCard = UIHelper::createHandCardImageView(viewChairID, bCardData[j]);
                    pCard->setAnchorPoint(Vec2(0, 0));
                    pCard->setPosition(Vec2(0, y - 20));
                    pCard->setLocalZOrder(j);
                    pHandCard1->addChild(pCard);
                }
                break;
            }
            case 2: {
                ui::Layout *pHandCard2 = dynamic_cast<ui::Layout *>(UIHelper::seekNodeByName(m_PlayerPanel[viewChairID], "HandCard_2"));
                ui::Layout *pComb2 = dynamic_cast<ui::Layout *>(UIHelper::seekNodeByName(m_PlayerPanel[viewChairID], "comb_2"));  //组合
                pHandCard2->removeAllChildren();
                pComb2->removeAllChildren();
                int x = 1027;  //宽起始位置
                for (uint8_t j = 0; j < cbWeaveItemCount; j++) {
                    x -= 228;
                    tagWeaveItem weaveItem = WeaveItemArray[j];
                    Node *pWeaveNode = NULL;
                    if (weaveItem.cbWeaveKind == WIK_G) {
                        pWeaveNode = CSLoader::createNode("res/Gang0.csb");
                    }
                    if (weaveItem.cbWeaveKind == WIK_P) {
                        pWeaveNode = CSLoader::createNode("res/Peng0.csb");
                    }
                    assert(pWeaveNode != NULL);
                    pWeaveNode->setPosition(Vec2(x + 23, 0));
                    const std::string &strImagePath = UIHelper::getDiscardCardImagePath(2, weaveItem.cbCenterCard);
                    const std::string &strBackImagePath = UIHelper::getBackCardImagePath(2, weaveItem.cbCenterCard);
                    ui::ImageView *pImageRight = dynamic_cast<ui::ImageView *>(UIHelper::seekNodeByName(pWeaveNode, "Image_right"));
                    ui::ImageView *pImageLeft = dynamic_cast<ui::ImageView *>(UIHelper::seekNodeByName(pWeaveNode, "Image_left"));
                    ui::ImageView *pImageCenter = dynamic_cast<ui::ImageView *>(UIHelper::seekNodeByName(pWeaveNode, "Image_center"));
                    pImageRight->loadTexture(strImagePath);
                    pImageCenter->loadTexture(strImagePath);
                    pImageLeft->loadTexture(strImagePath);
                    switch (m_GameEngine->switchViewChairID(weaveItem.cbProvideUser, m_MeChairID)) {
                        case 0: {
                            pImageCenter->loadTexture(strBackImagePath);
                            break;
                        }
                        case 1: {
                            pImageLeft->loadTexture(strBackImagePath);
                            break;
                        }
                        case 2: {
                            if (weaveItem.cbPublicCard == FALSE) {    //暗杠
                                pImageRight->loadTexture(strBackImagePath);
                                pImageCenter->loadTexture(strBackImagePath);
                                pImageLeft->loadTexture(strBackImagePath);
                            }
                            break;
                        }
                        case 3: {
                            pImageRight->loadTexture(strBackImagePath);
                            break;
                        }
                        default:
                            break;
                    }
                    pComb2->addChild(pWeaveNode);
                }
                for (uint8_t j = 0; j < MAX_COUNT - 1 - (3 * cbWeaveItemCount); j++) {
                    x -= 76;
                    ui::ImageView *pCard = UIHelper::createHandCardImageView(viewChairID, bCardData[j]);
                    pCard->setAnchorPoint(Vec2(0, 0));
                    pCard->setPosition(Vec2(x, 0));
                    pHandCard2->addChild(pCard);
                }
                break;
            }
            case 3: {
                ui::Layout *pHandCard3 = dynamic_cast<ui::Layout *>(UIHelper::seekNodeByName(m_PlayerPanel[viewChairID], "HandCard_3"));
                ui::Layout *pComb3 = dynamic_cast<ui::Layout *>(UIHelper::seekNodeByName(m_PlayerPanel[viewChairID], "comb_3"));  //组合
                pHandCard3->removeAllChildren();
                pComb3->removeAllChildren();
                int y = 0;  //高起始位置
                for (uint8_t j = 0; j < cbWeaveItemCount; j++) {
                    tagWeaveItem weaveItem = WeaveItemArray[j];
                    Node *pWeaveNode = NULL;
                    if (weaveItem.cbWeaveKind == WIK_G) {
                        pWeaveNode = CSLoader::createNode("res/Gang1.csb");
                    }
                    if (weaveItem.cbWeaveKind == WIK_P) {
                        pWeaveNode = CSLoader::createNode("res/Peng1.csb");
                    }
                    assert(pWeaveNode != NULL);
                    pWeaveNode->setPosition(Vec2(0, y - 10));
                    const std::string &strImagePath = UIHelper::getDiscardCardImagePath(3, weaveItem.cbCenterCard);
                    const std::string &strBackImagePath = UIHelper::getBackCardImagePath(3, weaveItem.cbCenterCard);
                    ui::ImageView *pImageRight = dynamic_cast<ui::ImageView *>(UIHelper::seekNodeByName(pWeaveNode, "Image_right"));
                    ui::ImageView *pImageLeft = dynamic_cast<ui::ImageView *>(UIHelper::seekNodeByName(pWeaveNode, "Image_left"));
                    ui::ImageView *pImageCenter = dynamic_cast<ui::ImageView *>(UIHelper::seekNodeByName(pWeaveNode, "Image_center"));
                    pImageRight->loadTexture(strImagePath);
                    pImageCenter->loadTexture(strImagePath);
                    pImageLeft->loadTexture(strImagePath);
                    switch (m_GameEngine->switchViewChairID(weaveItem.cbProvideUser, m_MeChairID)) {
                        case 0: {
                            pImageRight->loadTexture(strBackImagePath);
                            break;
                        }
                        case 1: {
                            pImageCenter->loadTexture(strBackImagePath);
                            break;
                        }
                        case 2: {
                            pImageLeft->loadTexture(strBackImagePath);
                            break;
                        }
                        case 3: {
                            if (weaveItem.cbPublicCard == FALSE) {    //暗杠
                                pImageRight->loadTexture(strBackImagePath);
                                pImageCenter->loadTexture(strBackImagePath);
                                pImageLeft->loadTexture(strBackImagePath);
                            }
                            break;
                        }
                        default:
                            break;
                    }
                    pComb3->addChild(pWeaveNode);
                    y += 160;
                }
                for (uint8_t j = 0; j < MAX_COUNT - 1 - (3 * cbWeaveItemCount); j++) {
                    ui::ImageView *pCard = UIHelper::createHandCardImageView(viewChairID, bCardData[j]);
                    pCard->setAnchorPoint(Vec2(0, 0));
                    pCard->setPosition(Vec2(0, y - 20));
                    pCard->setLocalZOrder(MAX_COUNT - j);
                    pHandCard3->addChild(pCard);
                    y += 60;
                }
                break;
            }
            default:
                break;
        }
    }
    return true;
}

/**
 * 播放特效
 * @param cbViewID
 * @param cbOperateCode
 * @param bZm
 * @return
 */
bool GameLayer::showAndPlayOperateEffect(uint8_t cbViewID, uint8_t cbOperateCode, bool bZm) {
    std::string strEffect("");
    switch (cbOperateCode) {
        case WIK_NULL:
            break;
        case WIK_P: 
            strEffect = "res/EffectPeng.csb";
            break;
        case WIK_G: 
            strEffect = "res/EffectGang.csb";
            break;
        case WIK_H: 
            strEffect = (bZm ? "res/EffectZm.csb" : "res/EffectHu.csb");
            break;
        default:
            break;
    }
    if (strEffect != "") {
        Vec2 pos;
        switch (cbViewID) {
            case 0:    //自己操作反馈
                pos = { 640.0f , 200.0f };
                break;
            case 1: 
                pos = { 440.0f , 360.0f };
                break;
            case 2: 
                pos = { 640.0f , 520.0f };
                break;
            case 3: 
                pos = { 840.0f , 360.0f };
                break;
            default:
                break;
        }
        std::string strNodeName = "EffectNode";
        Node *pEffectNode = CSLoader::createNode(strEffect);
        pEffectNode->setPosition(pos);
        cocostudio::timeline::ActionTimeline *action = CSLoader::createTimeline(strEffect);
        action->gotoFrameAndPlay(0, false);
        pEffectNode->setName(strNodeName);
        m_rootNode->addChild(pEffectNode);
        if (cbOperateCode != WIK_H) {    //胡牌不自动删除动画
            action->setLastFrameCallFunc(CC_CALLBACK_0(GameLayer::removeEffectNode, this, strNodeName));
        }
        pEffectNode->runAction(action);
    }
    return true;
}

/**
 * 移除动画节点
 * @param strNodeName
 */
void GameLayer::removeEffectNode(std::string strNodeName) {
    std::vector<Node *> aChildren;
    UIHelper::getChildren(m_rootNode, strNodeName, aChildren);
    for (auto &subChild : aChildren) {
        subChild->removeFromParent();
    }
    aChildren.clear();
}

/**
 * 显示、更新桌上的牌
 * @return
 */
bool GameLayer::showAndUpdateDiscardCard() {
    for (uint8_t cbChairID = 0; cbChairID < m_GameEngine->getPlayerCount(); cbChairID++) {
        uint8_t cbViewID = m_GameEngine->switchViewChairID(cbChairID, m_MeChairID);
        switch (cbViewID) {
            case 0: {
                ui::Layout *pDiscardCard0 = dynamic_cast<ui::Layout *>(this->getChildByName( "DiscardCard_0" ));
                pDiscardCard0->removeAllChildren();
                uint8_t bDiscardCount = m_cbDiscardCount[cbChairID];
                float x = 0;
                float y = 0;
                for (uint8_t i = 0; i < bDiscardCount; i++) {
                    uint8_t col = static_cast<uint8_t>(i % 12);
                    uint8_t row = static_cast<uint8_t>(i / 12);
                    x = (col == 0) ? 0 : x;  //X复位
                    y = row * 90;
                    ui::ImageView *pCard0 = UIHelper::createDiscardCardImageView(cbViewID, m_cbDiscardCard[cbChairID][i]);
                    pCard0->setAnchorPoint(Vec2(0, 0));
                    pCard0->setPosition(Vec2(x, y));
                    pCard0->setLocalZOrder(10 - row);
                    x += 76;
                    pDiscardCard0->addChild(pCard0);
                }
                pDiscardCard0->setScale(0.45f, 0.45f);
                break;
            }
            case 1: {
                //显示出的牌
                ui::Layout *pDiscardCard1 = dynamic_cast<ui::Layout *>(this->getChildByName( "DiscardCard_1" ));
                pDiscardCard1->removeAllChildren();
                uint8_t bDiscardCount = m_cbDiscardCount[cbChairID];
                float x = 0;
                float y = 0;
                for (uint8_t i = 0; i < bDiscardCount; i++) {
                    uint8_t col = static_cast<uint8_t>(i % 11);
                    uint8_t row = static_cast<uint8_t>(i / 11);
                    y = (col == 0) ? 0 : y;  //X复位
                    x = 116 * row;
                    ui::ImageView *pCard1 = UIHelper::createDiscardCardImageView(cbViewID, m_cbDiscardCard[cbChairID][i]);
                    pCard1->setAnchorPoint(Vec2(0, 0));
                    pCard1->setPosition(Vec2(x, 740 - y));
                    pCard1->setLocalZOrder(col);
                    y += 74;
                    pDiscardCard1->addChild(pCard1);
                }
                pDiscardCard1->setScale(0.5, 0.5);
                break;
            }
            case 2: {
                ui::Layout *pDiscardCard2 = dynamic_cast<ui::Layout *>(this->getChildByName( "DiscardCard_2" ));
                pDiscardCard2->removeAllChildren();
                uint8_t bDiscardCount = m_cbDiscardCount[cbChairID];
                float x = 0;
                float y = 0;
                for (uint8_t i = 0; i < bDiscardCount; i++) {
                    uint8_t col = static_cast<uint8_t>(i % 12);
                    uint8_t row = static_cast<uint8_t>(i / 12);
                    x = (col == 0) ? 0 : x;  //X复位
                    y = 90 - row * 90;
                    ui::ImageView *pCard2 = UIHelper::createDiscardCardImageView(cbViewID, m_cbDiscardCard[cbChairID][i]);
                    pCard2->setAnchorPoint(Vec2(0, 0));
                    pCard2->setPosition(Vec2(x, y));
                    pCard2->setLocalZOrder(row);
                    x += 76;
                    pDiscardCard2->addChild(pCard2);
                }
                pDiscardCard2->setScale(0.45f, 0.45f);

                break;
            }
            case 3: {
                //显示出的牌
                ui::Layout *pDiscardCard3 = dynamic_cast<ui::Layout *>(this->getChildByName( "DiscardCard_3" ));
                pDiscardCard3->removeAllChildren();
                uint8_t bDiscardCount = m_cbDiscardCount[cbChairID];
                float x = 0;
                float y = 0;
                for (uint8_t i = 0; i < bDiscardCount; i++) {
                    uint8_t col = static_cast<uint8_t>(i % 11);
                    uint8_t row = static_cast<uint8_t>(i / 11);
                    y = (col == 0) ? 0 : y;  //X复位
                    x = 240 - (116 * row);
                    ui::ImageView *pCard3 = UIHelper::createDiscardCardImageView(cbViewID, m_cbDiscardCard[cbChairID][i]);
                    pCard3->setAnchorPoint(Vec2(0, 0));
                    pCard3->setPosition(Vec2(x, y));
                    pCard3->setLocalZOrder(20 - col);
                    y += 74;
                    pDiscardCard3->addChild(pCard3);
                }
                pDiscardCard3->setScale(0.5, 0.5);
                break;
            }
            default:
                break;
        }
    }
    return true;
}

/**
 * 出牌
 * @param ref
 * @param eventType
 */
void GameLayer::onCardTouch(Ref *ref, ui::Widget::TouchEventType eventType) {
    if (m_pOutCard != NULL && m_pOutCard != ref) return;
    if (m_bOperate) {
        ui::ImageView *pCard = dynamic_cast<ui::ImageView *>(ref);
        ui::Layout *pHandCard0 = dynamic_cast<ui::Layout *>(UIHelper::seekNodeByName(m_PlayerPanel[0], "HandCard_0"));
        const auto &aChildren = pHandCard0->getChildren();
        for (auto &subWidget : aChildren) {
            Node *child = dynamic_cast<Node *>(subWidget);
            if (pCard != child) {
                child->setPositionY(0.0f);
            }
        }
        ui::Layout *pRecvHandCard0 = dynamic_cast<ui::Layout *>(UIHelper::seekNodeByName(m_PlayerPanel[0], "RecvHandCard_0"));
        const auto &bChildren = pRecvHandCard0->getChildren();
        for (auto &subWidget : bChildren) {
            Node *child = dynamic_cast<Node *>(subWidget);
            if (pCard != child) {
                child->setPositionY(0.0f);
            }
        }
        if (pCard != NULL) {
            std::string btCardName = pCard->getName();
            switch (eventType) {
                case ui::Widget::TouchEventType::BEGAN: {
                    m_pOutCard = dynamic_cast<Node *>(ref);         //正在出牌状态
                    m_bMove = false;                                //是否移动牌状态
                    m_startVec = pCard->getPosition();              //记录开始位置
                    //听牌判断
                    uint8_t cbCardData = static_cast<uint8_t>(pCard->getTag());   //牌
                    uint8_t cbWeaveItemCount = m_cbWeaveItemCount[m_MeChairID];
                    tagWeaveItem *pTagWeaveItem = m_WeaveItemArray[m_MeChairID];
                    uint8_t cbTingCard[MAX_INDEX];
                    memset(&cbTingCard, 0, sizeof(cbTingCard));
                    uint8_t *cbCardIndex = m_cbCardIndex[m_MeChairID];
                    memcpy(&cbTingCard, cbCardIndex, sizeof(cbTingCard));   //内存拷贝
                    cbTingCard[GameLogic::switchToCardIndex(cbCardData)]--;   //移除要出的牌进行分析
                    showTingResult(cbTingCard, pTagWeaveItem, cbWeaveItemCount);
                    break;
                }
                case ui::Widget::TouchEventType::MOVED: {
                    Vec2 a = pCard->getTouchBeganPosition();
                    Vec2 b = pCard->getTouchMovePosition();
                    pCard->setPosition(m_startVec + (b - a));       //移动
                    if (b.y - a.y > 60 || abs(b.x - a.x) > 30) {    //移动判定
                        m_bMove = true;
                    }
                    break;
                }
                case ui::Widget::TouchEventType::CANCELED:
                case ui::Widget::TouchEventType::ENDED: {
                    m_pOutCard = NULL;                             //结束出牌状态
                    Vec2 endVec = pCard->getPosition();
                    if (endVec.y - m_startVec.y > 118) {
                        CCLOG("out card");
                        CMD_C_OutCard OutCard;
                        memset(&OutCard, 0, sizeof(CMD_C_OutCard));
                        OutCard.cbCardData = static_cast<uint8_t >(pCard->getTag());
                        m_GameEngine->onUserOutCard(OutCard);
                    } else {
                        if (m_startVec.y == m_cardPosY) {                               //正常状态
                            if (m_bMove) {
                                pCard->setPosition(m_startVec);                         //移动
                            } else {
                                pCard->setPosition(m_startVec + Vec2(0, m_outY));       //移动
                            }
                        } else if (m_startVec.y == m_cardPosY + m_outY) {               //选牌状态
                            Vec2 a = pCard->getTouchBeganPosition();                    //触摸上半部分，撤销出牌
                            if (a.y > 118) {
                                pCard->setPosition(m_startVec - Vec2(0, m_outY));
                            } else {                                                   //触摸下半部分，出牌
                                pCard->setPosition(m_startVec);
                                CMD_C_OutCard OutCard;
                                memset(&OutCard, 0, sizeof(CMD_C_OutCard));
                                OutCard.cbCardData = static_cast<uint8_t>(pCard->getTag());
                                m_GameEngine->onUserOutCard(OutCard);
                            }
                        } else {
                            pCard->setPosition(m_startVec);    //移动
                        }
                    }
                    break;
                }

            }
        }
    }
}


