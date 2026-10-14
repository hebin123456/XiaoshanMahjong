//
// Created by farmer on 2018/7/4.
//

#ifndef COCOSTUDIO_MAHJONG_GAMELAYER_H
#define COCOSTUDIO_MAHJONG_GAMELAYER_H

#include "IDialog.h"
#include "UIHelper.h"
#include "IPlayer.h"
#include "GameCmd.h"
#include "Utility.h"
#include "GameEngine.h"
#include "GameLogic.h"
#include "GameOverDlg.h"

class GameLayer : public IDialog<Layer>, IGameEngineEventListener {
    friend class GameOverDlg;
private:
    GameEngine *m_GameEngine;           //游戏引擎
    Node *m_FaceFrame[GAME_PLAYER];     //头像信息节点
    Node *m_PlayerPanel[GAME_PLAYER];   //玩家牌的区域
    Node *m_pOperateNotifyGroup;        //操作组
    Node* m_pOutCard;                   //正在出牌的节点
    ui::Text *m_pTextCardNum;           //剩余牌数量
    Node *m_pCardWall;                  //桌面牌墙（四边各17墩的俯视牌堆）
    int m_iWallFirstSlot;               //骰子定出的起牌槽位（第一张被拿走的牌在牌墙环上的位置）
    bool m_bDiceConfirmed;              //本局骰子是否已确认（防止重复触发发牌）
    uint8_t m_MeChairID;                //自己的位置
    uint8_t m_iOutCardTimeOut;          //操作时间
//游戏变量
private:
    tagWeaveItem m_WeaveItemArray[GAME_PLAYER][MAX_WEAVE];                 //组合
    uint8_t m_cbCardIndex[GAME_PLAYER][MAX_INDEX];                         //玩家牌
    uint8_t m_cbWeaveItemCount[GAME_PLAYER];                               //组合数目
    uint8_t m_cbDiscardCount[GAME_PLAYER];                                 //丢弃数目
    uint8_t m_cbDiscardCard[GAME_PLAYER][MAX_DISCARD];                     //丢弃记录
    uint8_t m_cbLeftCardCount;                                             //剩余
    uint8_t m_cbBankerChair;                                               //庄
    bool m_bOperate;                                                       //是否可操作
    bool m_bMove;                                                          //是否移动牌
    Vec2 m_startVec = Vec2(0,0);                                           //记录位置
    float m_outY = 30;                                                     //偏移
    float m_cardPosY = 0.0f;                                               //偏移

public:
    GameLayer();    // 构造函数
    virtual ~GameLayer();   //析构
    CREATE_FUNC(GameLayer);

protected:
    Button* m_btnExit;
    Button* m_btnSetting;
    virtual const char* csbName() const {return "res/GameLayer.csb";}
    virtual void onUILoaded();

public:
    void initGame();                    //初始化游戏变量
    void sendCardTimerUpdate(float f);  //倒计时
    void onCardTouch(Ref *ref, ui::Widget::TouchEventType eventType);   //触摸牌的事件

private:
    virtual bool onUserEnterEvent(IPlayer *pIPlayer);            //玩家进入事件
    virtual bool onGameStartEvent(CMD_S_GameStart GameStart);    //游戏开始事件
    virtual bool onSendCardEvent(CMD_S_SendCard SendCard);       //发牌事件
    virtual bool onOutCardEvent(CMD_S_OutCard OutCard);          //出牌事件
    virtual bool onOperateNotifyEvent(CMD_S_OperateNotify OperateNotify);   //操作通知事件
    virtual bool onOperateResultEvent(CMD_S_OperateResult OperateResult);   //操作结果事件
    virtual bool onGameEndEvent(CMD_S_GameEnd& GameEnd);                     //移除特效

private:
    bool showAndUpdateHandCard();                                                   //显示手上的牌
    bool showAndUpdateDiscardCard();                                                //显示桌上的牌
    bool showSendCard(CMD_S_SendCard SendCard);                                     //发牌显示
    bool showOperateNotify(CMD_S_OperateNotify OperateNotify);                      //显示操作通知
    bool showAndPlayOperateEffect(uint8_t cbViewID,uint8_t cbOperateCode, bool bZm);//播放特效
    bool showTingResult(const uint8_t cbCardIndex[MAX_INDEX], tagWeaveItem WeaveItem[], uint8_t cbWeaveCount);   //显示听牌的结果
    bool showAndUpdateUserScore(const int64_t lGameScoreTable[GAME_PLAYER]);             //更新分数
    void showCardWall(uint8_t cbBankerChair, uint8_t cbDice1, uint8_t cbDice2);     //绘制四边牌墙（按骰子留出起牌缺口）
    void popCardWallOne();                                                          //摸牌后牌墙起牌口少一张
    void showJokerCard(uint8_t cbCardData);                                         //财神牌翻出后摆在牌墙旁边
    Vec2 wallSlotPos(int iSlot);                                                    //牌墙槽位坐标（俯视图）
    Node *buildDieNode();                                                           //构建一颗骰子
    void setDieFace(Node *pDie, uint8_t cbValue);                                   //设置骰子点数
    void removeDiceNodes();                                                         //清理骰子相关节点与定时任务
    void playDiceAnimation(CMD_S_GameStart GameStart);                              //播放庄家掷骰动画
    void showDiceRollButton(CMD_S_GameStart GameStart);                             //人是庄家时显示掷骰按钮
    void showBankerDiceFlow(CMD_S_GameStart GameStart);                             //首局：每人掷骰定庄流程（AI自动，真人按钮）
    void playBankerDiceStep(CMD_S_GameStart GameStart);                             //定庄流程：轮到第几家掷
    void rollBankerDiceAnim(uint8_t cbChair, CMD_S_GameStart GameStart);            //定庄流程：滚动一家掷骰动画并记录点数
    void finishGameStartUI(CMD_S_GameStart GameStart);                              //掷骰结束后：亮手牌、翻财神、开始发牌
    int m_iBankerDiceStep;                                                          //定庄流程当前轮到第几家掷骰

private:
    void removeEffectNode(std::string strNodeName); //移除特效
};


#endif //COCOSTUDIO_MAHJONG_GAMELAYER_H
