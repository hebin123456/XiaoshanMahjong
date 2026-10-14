//
// Created by farmer on 2018/7/8.
//

#include "GameConfig.h"

static GameConfig* m_pGameConfig = NULL;

GameConfig *GameConfig::getInstance() {     //单例创建全局唯一的GameConfig
    if (m_pGameConfig == NULL){
        m_pGameConfig = new GameConfig();
    }
    return m_pGameConfig;
}

GameConfig::GameConfig() {
    m_EffectsVolume = 0;
    loadConfig();
}

GameConfig::~GameConfig() {

}

/**
 * 加载配置
 */
void GameConfig::loadConfig() {
    m_EffectsVolume = UserDefault::getInstance()->getFloatForKey("EffectsVolume", 0.8f);
    //萧山麻将规则开关（默认开启：捉冲、吃三摊、吃牌；默认不强制爆头）
    m_bAllowChiHu = UserDefault::getInstance()->getBoolForKey("Rule_AllowChiHu", true);
    m_bJokerBaoTou = UserDefault::getInstance()->getBoolForKey("Rule_JokerBaoTou", false);
    m_bAllowChiSanTan = UserDefault::getInstance()->getBoolForKey("Rule_AllowChiSanTan", true);
}

/**
 * 保存配置
 */
void GameConfig::saveConfig() {
    UserDefault::getInstance()->setFloatForKey("EffectsVolume", m_EffectsVolume);
    UserDefault::getInstance()->setBoolForKey("Rule_AllowChiHu", m_bAllowChiHu);
    UserDefault::getInstance()->setBoolForKey("Rule_JokerBaoTou", m_bJokerBaoTou);
    UserDefault::getInstance()->setBoolForKey("Rule_AllowChiSanTan", m_bAllowChiSanTan);
}

