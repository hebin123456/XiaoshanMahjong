//
// Created by farmer on 2018/7/8.
//

#ifndef COCOSTUDIO_MAHJONG_GAMECONFIG_H
#define COCOSTUDIO_MAHJONG_GAMECONFIG_H

#include "cocos2d.h"

using namespace cocos2d;

class GameConfig {

public: //音量
    float m_EffectsVolume;

public: //萧山麻将规则开关
    bool m_bAllowChiHu;        //允许捉冲（点炮胡），关闭则只能自摸
    bool m_bJokerBaoTou;       //有财神必须爆头（财神作将）
    bool m_bAllowChiSanTan;    //允许吃三摊（第三个明牌组合触发承包）

public: //AI 强度：0 入门 1 普通 2 高手
    int m_iAiLevel;

public: //萧山麻将：起始积分与基本分（底分）
    int m_iStartScore;    //起始积分：50 或 100
    int m_iBaseFan;       //基本分（底分）：1 / 2 / 4 / 8，结算=番数×底分
    int m_iFanCap;        //番数封顶：16 / 32 / 64（连庄倍数与番型合计的上限，默认32）

public: //打牌语音性别：0 女声 1 男声（默认男声）
    int m_iVoiceSex;

private:
    GameConfig();
    ~GameConfig();

public:
    void loadConfig();  //重新加载配置
    void saveConfig();  //保存配置

public:
    /**
     * 获取游戏配置单例
     * @return
     */
    static GameConfig *getInstance();
};


#endif //COCOSTUDIO_MAHJONG_GAMECONFIG_H
