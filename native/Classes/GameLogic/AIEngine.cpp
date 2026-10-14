//
// Created by farmer on 2018/7/5.
//

#include "AIEngine.h"
#include "IPlayer.h"
#include "DelayCall.h"
#include "GameConfig.h"

AIEngine::AIEngine() {
    m_GameEngine = GameEngine::GetGameEngine();
    m_cbSendCardData = 0;
    m_MeChairID = INVALID_CHAIR;
    initGame();
}

AIEngine::~AIEngine() {
    cocos2d::log("~AIEngine");
}

/**
 * 初始化游戏变量
 */
void AIEngine::initGame(){
    m_cbSendCardData = 0;
    m_cbLeftCardCount = 0;
    m_cbBankerChair = INVALID_CHAIR;
    memset(&m_cbWeaveItemCount, 0, sizeof(m_cbWeaveItemCount));
    memset(&m_cbDiscardCount, 0, sizeof(m_cbDiscardCount));
    for (uint8_t i = 0; i < GAME_PLAYER; i++) {
        memset(m_cbCardIndex[i], 0, sizeof(m_cbCardIndex[i]));
        memset(m_WeaveItemArray[i], 0, sizeof(m_WeaveItemArray[i]));
        memset(m_cbDiscardCard[i], 0, sizeof(m_cbDiscardCard[i]));
    }
}

void AIEngine::setIPlayer(IPlayer *pIPlayer) {
    m_MePlayer = pIPlayer;
}

bool AIEngine::onUserEnterEvent(IPlayer *pIPlayer) {
    m_MeChairID = m_MePlayer->getChairID();
    return true;
}

bool AIEngine::onGameStartEvent(CMD_S_GameStart GameStart) {
    cocos2d::log("机器人接收到游戏开始事件");
    initGame();
    m_cbLeftCardCount = GameStart.cbLeftCardCount;
    m_cbBankerChair = GameStart.cbBankerUser;
    GameLogic::switchToCardIndex(GameStart.cbCardData, MAX_COUNT - 1, m_cbCardIndex[m_MeChairID]);
    return true;
}

bool AIEngine::onSendCardEvent(CMD_S_SendCard SendCard) {
    if (SendCard.cbCurrentUser == m_MeChairID) { //出牌
        m_cbLeftCardCount--;
        if (SendCard.cbCurrentUser == m_MeChairID) {
            cocos2d::log("机器人接收到发牌事件");
            m_cbCardIndex[m_MeChairID][GameLogic::switchToCardIndex(SendCard.cbCardData)]++;
        }
        m_cbSendCardData = SendCard.cbCardData;
        if (SendCard.cbActionMask != WIK_NULL) {//发的牌存在动作，模拟发送操作通知
            CMD_S_OperateNotify OperateNotify;
            memset(&OperateNotify, 0, sizeof(CMD_S_OperateNotify));
            OperateNotify.cbActionMask = SendCard.cbActionMask;
            OperateNotify.cbActionCard = SendCard.cbCardData;
            OperateNotify.cbGangCount = SendCard.cbGangCount;
            memcpy(OperateNotify.cbGangCard, SendCard.cbGangCard, sizeof(OperateNotify.cbGangCard));
            onOperateNotifyEvent(OperateNotify);
        } else {
            //按 AI 强度选择出牌（默认打法为直接打出刚摸的牌）
            m_cbSendCardData = chooseDiscardCard();
            DelayCall::add([this]() { sendCard(); }, time(NULL) % 2 + 0.8f);
        }
    }
    return true;
}

/**
 * 出牌事件
 * @param OutCard
 * @return
 */
bool AIEngine::onOutCardEvent(CMD_S_OutCard OutCard) {
    cocos2d::log("出牌人 %d，坐席 %d", OutCard.cbOutCardUser, m_MeChairID);
    if (OutCard.cbOutCardUser == m_MeChairID) {
        cocos2d::log("机器人接收到出牌事件");
        m_cbCardIndex[m_MeChairID][GameLogic::switchToCardIndex(OutCard.cbOutCardData)]--;
    }
    m_cbDiscardCard[OutCard.cbOutCardUser][m_cbDiscardCount[OutCard.cbOutCardUser]++] = OutCard.cbOutCardData;
    return true;
}

/**
 * 操作通知事件
 * @param OperateNotify
 * @return
 */
bool AIEngine::onOperateNotifyEvent(CMD_S_OperateNotify OperateNotify) {
    cocos2d::log("机器人接收到操作通知事件");
    if (OperateNotify.cbActionMask == WIK_NULL) {
        return true; //无动作
    }
    CMD_C_OperateCard OperateCard;
    memset(&OperateCard, 0, sizeof(CMD_C_OperateCard));     //重置内存
    OperateCard.cbOperateUser = m_MeChairID;   //操作的玩家
    if ((OperateNotify.cbActionMask & WIK_H) != 0) {        //胡的优先级最高
        OperateCard.cbOperateCode = WIK_H;
        OperateCard.cbOperateCard = OperateNotify.cbActionCard;
    } else if ((OperateNotify.cbActionMask & WIK_G) != 0) { //杠的优先级第二
        OperateCard.cbOperateCode = WIK_G;
        OperateCard.cbOperateCard = OperateNotify.cbGangCard[0];//杠第一个
    } else if ((OperateNotify.cbActionMask & WIK_P) != 0) { //碰的优先级第三
        OperateCard.cbOperateCode = WIK_P;
        OperateCard.cbOperateCard = OperateNotify.cbActionCard;
    }
    return m_GameEngine->onUserOperateCard(OperateCard);
}

/**
 * 操作结果事件
 * @param OperateResult
 * @return
 */
bool AIEngine::onOperateResultEvent(CMD_S_OperateResult OperateResult) {
    cocos2d::log("机器人接收到操作结果事件");
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
            if (OperateResult.cbOperateUser == m_MeChairID) { //自己出牌操作
                uint8_t cbReomveCard[] = {OperateResult.cbOperateCard, OperateResult.cbOperateCard};
                GameLogic::removeCard(m_cbCardIndex[OperateResult.cbOperateUser], cbReomveCard, sizeof(cbReomveCard));
                m_cbSendCardData = chooseDiscardCard();
                DelayCall::add([this]() { sendCard(); }, time(NULL) % 2 + 0.5f);
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
    return true;
}

/**
 * 结束
 * @param GameEnd
 * @return
 */
bool AIEngine::onGameEndEvent(CMD_S_GameEnd& GameEnd) {
    cocos2d::log("机器人接收到游戏结束事件");
    return true;
}

/**
 * 出牌
 * @param f
 */
void AIEngine::sendCard() {
    cocos2d::log("机器人出牌:%x",m_cbSendCardData);
    CMD_C_OutCard OutCard;
    memset(&OutCard, 0, sizeof(CMD_C_OutCard));
    OutCard.cbCardData = m_cbSendCardData;
    m_GameEngine->onUserOutCard(OutCard);
}

/**
 * 按 AI 强度选择要打出的牌
 * 0 入门：直接打出刚摸到的牌
 * 1 普通：结构评估，优先打出孤张，保留对子/搭子，不主动打财神
 * 2 高手：在普通基础上偏好打出已现身的「熟张」（降低放炮风险）
 * @return 要打出的牌
 */
uint8_t AIEngine::chooseDiscardCard() {
    int level = GameConfig::getInstance()->m_iAiLevel;
    if (level <= 0) { return m_cbSendCardData; }   //入门：直接打出摸到的牌

    const uint8_t *counts = m_cbCardIndex[m_MeChairID];
    uint8_t bestCard = 0;
    int bestScore = 0x7FFFFFFF;
    bool found = false;
    for (uint8_t i = 0; i < MAX_INDEX; i++) {
        if (counts[i] == 0) { continue; }
        uint8_t card = GameLogic::switchToCardData(i);
        if (card == JOKER_CARD) { continue; }      //财神为万能牌，不主动打出

        uint8_t value = static_cast<uint8_t>(i % 9);
        int score = counts[i] * 30;                //对子/刻子价值高，优先保留
        if (i < 27) {                              //数牌：看同花色相邻搭子
            if (value >= 1 && counts[i - 1] > 0) { score += 12; }
            if (value <= 7 && counts[i + 1] > 0) { score += 12; }
            if (value >= 2 && counts[i - 2] > 0) { score += 6; }
            if (value <= 6 && counts[i + 2] > 0) { score += 6; }
        }
        if (level >= 2) {                          //高手：熟张更安全，降低保留倾向
            bool bSafe = false;
            for (uint8_t p = 0; p < GAME_PLAYER && !bSafe; p++) {
                if (p == m_MeChairID) { continue; }
                for (uint8_t d = 0; d < m_cbDiscardCount[p]; d++) {
                    if (m_cbDiscardCard[p][d] == card) { bSafe = true; break; }
                }
            }
            if (bSafe) { score -= 20; }
        }
        if (!found || score < bestScore) {
            bestScore = score;
            bestCard = card;
            found = true;
        }
    }
    if (!found) { return m_cbSendCardData; }       //手上只剩财神时，只能打出摸到的牌
    return bestCard;
}

