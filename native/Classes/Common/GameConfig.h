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
