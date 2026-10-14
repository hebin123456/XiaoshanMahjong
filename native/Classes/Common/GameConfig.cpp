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
    //萧山麻将规则开关（默认开启：捉冲、吃三摊；默认不强制爆头；AI 默认普通）
    m_bAllowChiHu = UserDefault::getInstance()->getBoolForKey("Rule_AllowChiHu", true);
    m_bJokerBaoTou = UserDefault::getInstance()->getBoolForKey("Rule_JokerBaoTou", false);
    m_bAllowChiSanTan = UserDefault::getInstance()->getBoolForKey("Rule_AllowChiSanTan", true);
    m_iAiLevel = UserDefault::getInstance()->getIntegerForKey("Rule_AiLevel", 1);
    //起始积分（默认50）与基本分（默认1）
    int iStartScore = UserDefault::getInstance()->getIntegerForKey("Rule_StartScore", 50);
    m_iStartScore = (iStartScore == 100) ? 100 : 50;
    int iBaseFan = UserDefault::getInstance()->getIntegerForKey("Rule_BaseFan", 1);
    m_iBaseFan = (iBaseFan == 2 || iBaseFan == 4 || iBaseFan == 8) ? iBaseFan : 1;
    //番数封顶（默认32）
    int iFanCap = UserDefault::getInstance()->getIntegerForKey("Rule_FanCap", 32);
    m_iFanCap = (iFanCap == 16 || iFanCap == 64) ? iFanCap : 32;
}

/**
 * 保存配置
 */
void GameConfig::saveConfig() {
    UserDefault::getInstance()->setFloatForKey("EffectsVolume", m_EffectsVolume);
    UserDefault::getInstance()->setBoolForKey("Rule_AllowChiHu", m_bAllowChiHu);
    UserDefault::getInstance()->setBoolForKey("Rule_JokerBaoTou", m_bJokerBaoTou);
    UserDefault::getInstance()->setBoolForKey("Rule_AllowChiSanTan", m_bAllowChiSanTan);
    UserDefault::getInstance()->setIntegerForKey("Rule_AiLevel", m_iAiLevel);
    UserDefault::getInstance()->setIntegerForKey("Rule_StartScore", m_iStartScore);
    UserDefault::getInstance()->setIntegerForKey("Rule_BaseFan", m_iBaseFan);
    UserDefault::getInstance()->setIntegerForKey("Rule_FanCap", m_iFanCap);
}

