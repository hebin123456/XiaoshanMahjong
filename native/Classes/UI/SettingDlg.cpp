//
// Created by farmer on 2018/7/8.
//

#include "SettingDlg.h"
#include "GameConfig.h"

using namespace std;

SettingDlg::SettingDlg() : IDialog()
{
    RegDialogCtrl("Button_Close", m_btnClose);
    RegDialogCtrl("Slider_Volume", m_pSliderVolume);
    for (int i = 0; i < RULE_COUNT; i++) { m_ruleLabels[i] = nullptr; m_ruleValues[i] = nullptr; }
    m_aiLevelLabel = nullptr;
    m_startScoreLabel = nullptr;
    m_baseFanLabel = nullptr;
}
SettingDlg::~SettingDlg(){
}

void SettingDlg::onUILoaded() {
    m_rootNode->setScale(0.5f);
    m_rootNode->runAction(EaseElasticOut::create(ScaleTo::create(0.5f, 1.0f, 1.0f))); //动画
    m_rootNode->addChild(LayerColor::create(Color4B(0, 0, 0, 200)), -1);
    m_btnClose->addClickEventListener([this](Ref* sender){
        GameConfig::getInstance()->saveConfig();    //关闭前保存配置
        DialogManager::shared()->closeCurrentDialog();
    });
    m_pSliderVolume->setMaxPercent(100);
    m_pSliderVolume->setPercent(static_cast<int>(GameConfig::getInstance()->m_EffectsVolume * 100));
    //滑块移动事件
    m_pSliderVolume->addEventListener([this](Ref*, ui::Slider::EventType type) {
        switch (type) {
            case ui::Slider::EventType::ON_PERCENTAGE_CHANGED : {    //滑动条变化
                int currPercent = m_pSliderVolume->getPercent();    //当前百分比
                int maxPercent = m_pSliderVolume->getMaxPercent();  //最大百分比
                GameConfig::getInstance()->m_EffectsVolume = (float) currPercent / (float) maxPercent;
                break;
            }
            default:
                break;
        }
    });
    //萧山麻将规则开关
    buildRuleSwitches();
}

/**
 * 构建规则开关控件（点击切换开/关），排在音量滑块正下方
 */
void SettingDlg::buildRuleSwitches() {
    GameConfig *cfg = GameConfig::getInstance();
    m_ruleValues[0] = &cfg->m_bAllowChiHu;
    m_ruleValues[1] = &cfg->m_bJokerBaoTou;
    m_ruleValues[2] = &cfg->m_bAllowChiSanTan;

    //以音量滑块为锚点，把开关排在滑块正下方（同一坐标系，避免与 csb 布局冲突）
    Vec2 anchor = m_rootNode->convertToNodeSpace(
            m_pSliderVolume->getParent()->convertToWorldSpace(m_pSliderVolume->getPosition()));

    const float fLineStep = 42.0f;  //行距（紧凑排布：3规则+AI+起始积分+基本分 共6行）
    for (int i = 0; i < RULE_COUNT; i++) {
        Label *label = Label::createWithTTF("", "DFYuanW7-GB2312.ttf", 28);
        label->setAnchorPoint(Vec2(0.5f, 0.5f));
        label->setPosition(Vec2(anchor.x, anchor.y - 80.0f - i * fLineStep));
        label->setColor(Color3B(255, 255, 255));
        m_rootNode->addChild(label, 10);
        m_ruleLabels[i] = label;
        refreshRuleLabel(i);

        auto listener = EventListenerTouchOneByOne::create();
        listener->setSwallowTouches(true);
        listener->onTouchBegan = [this, i](Touch *touch, Event *) -> bool {
            Label *l = m_ruleLabels[i];
            Vec2 p = l->getParent()->convertToNodeSpace(touch->getLocation());
            return l->getBoundingBox().containsPoint(p);
        };
        listener->onTouchEnded = [this, i](Touch *touch, Event *) {
            Label *l = m_ruleLabels[i];
            Vec2 p = l->getParent()->convertToNodeSpace(touch->getLocation());
            if (l->getBoundingBox().containsPoint(p)) {
                *m_ruleValues[i] = !(*m_ruleValues[i]);   //切换开关
                refreshRuleLabel(i);
                GameConfig::getInstance()->saveConfig();  //立即持久化
            }
        };
        _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, label);
    }

    //AI 强度（点击循环切换：入门→普通→高手）
    m_aiLevelLabel = Label::createWithTTF("", "DFYuanW7-GB2312.ttf", 28);
    m_aiLevelLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
    m_aiLevelLabel->setPosition(Vec2(anchor.x, anchor.y - 80.0f - RULE_COUNT * fLineStep));
    m_aiLevelLabel->setColor(Color3B(255, 255, 255));
    m_rootNode->addChild(m_aiLevelLabel, 10);
    refreshAiLevelLabel();

    auto aiListener = EventListenerTouchOneByOne::create();
    aiListener->setSwallowTouches(true);
    aiListener->onTouchBegan = [this](Touch *touch, Event *) -> bool {
        Vec2 p = m_aiLevelLabel->getParent()->convertToNodeSpace(touch->getLocation());
        return m_aiLevelLabel->getBoundingBox().containsPoint(p);
    };
    aiListener->onTouchEnded = [this](Touch *touch, Event *) {
        Vec2 p = m_aiLevelLabel->getParent()->convertToNodeSpace(touch->getLocation());
        if (m_aiLevelLabel->getBoundingBox().containsPoint(p)) {
            GameConfig *cfg = GameConfig::getInstance();
            cfg->m_iAiLevel = (cfg->m_iAiLevel + 1) % 3;   //循环切换
            refreshAiLevelLabel();
            cfg->saveConfig();
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(aiListener, m_aiLevelLabel);

    //起始积分（点击切换：50 ↔ 100）
    m_startScoreLabel = Label::createWithTTF("", "DFYuanW7-GB2312.ttf", 28);
    m_startScoreLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
    m_startScoreLabel->setPosition(Vec2(anchor.x, anchor.y - 80.0f - (RULE_COUNT + 1) * fLineStep));
    m_startScoreLabel->setColor(Color3B(255, 235, 130));
    m_rootNode->addChild(m_startScoreLabel, 10);
    refreshStartScoreLabel();

    auto scoreListener = EventListenerTouchOneByOne::create();
    scoreListener->setSwallowTouches(true);
    scoreListener->onTouchBegan = [this](Touch *touch, Event *) -> bool {
        Vec2 p = m_startScoreLabel->getParent()->convertToNodeSpace(touch->getLocation());
        return m_startScoreLabel->getBoundingBox().containsPoint(p);
    };
    scoreListener->onTouchEnded = [this](Touch *touch, Event *) {
        Vec2 p = m_startScoreLabel->getParent()->convertToNodeSpace(touch->getLocation());
        if (m_startScoreLabel->getBoundingBox().containsPoint(p)) {
            GameConfig *cfg = GameConfig::getInstance();
            cfg->m_iStartScore = (cfg->m_iStartScore == 50) ? 100 : 50;    //50/100 循环
            refreshStartScoreLabel();
            cfg->saveConfig();
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(scoreListener, m_startScoreLabel);

    //基本分/起始倍数（点击循环：1→2→4→8→1）
    m_baseFanLabel = Label::createWithTTF("", "DFYuanW7-GB2312.ttf", 28);
    m_baseFanLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
    m_baseFanLabel->setPosition(Vec2(anchor.x, anchor.y - 80.0f - (RULE_COUNT + 2) * fLineStep));
    m_baseFanLabel->setColor(Color3B(255, 235, 130));
    m_rootNode->addChild(m_baseFanLabel, 10);
    refreshBaseFanLabel();

    auto fanListener = EventListenerTouchOneByOne::create();
    fanListener->setSwallowTouches(true);
    fanListener->onTouchBegan = [this](Touch *touch, Event *) -> bool {
        Vec2 p = m_baseFanLabel->getParent()->convertToNodeSpace(touch->getLocation());
        return m_baseFanLabel->getBoundingBox().containsPoint(p);
    };
    fanListener->onTouchEnded = [this](Touch *touch, Event *) {
        Vec2 p = m_baseFanLabel->getParent()->convertToNodeSpace(touch->getLocation());
        if (m_baseFanLabel->getBoundingBox().containsPoint(p)) {
            GameConfig *cfg = GameConfig::getInstance();
            cfg->m_iBaseFan = (cfg->m_iBaseFan >= 8) ? 1 : (cfg->m_iBaseFan * 2);  //1/2/4/8 循环
            refreshBaseFanLabel();
            cfg->saveConfig();
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(fanListener, m_baseFanLabel);
}

/**
 * 刷新规则开关显示文本
 * @param index
 */
void SettingDlg::refreshRuleLabel(int index) {
    static const char *kRuleTitles[RULE_COUNT] = {
            "允许捉冲(点炮胡)",
            "有财神必须爆头",
            "允许吃三摊(承包)",
    };
    if (index < 0 || index >= RULE_COUNT || m_ruleLabels[index] == nullptr) { return; }
    bool on = *m_ruleValues[index];
    m_ruleLabels[index]->setString(std::string(kRuleTitles[index]) + "：" + (on ? "开" : "关"));
    m_ruleLabels[index]->setColor(on ? Color3B(120, 255, 120) : Color3B(200, 200, 200));
}

/**
 * 刷新 AI 强度显示文本
 */
void SettingDlg::refreshAiLevelLabel() {
    static const char *kLevels[3] = {"入门", "普通", "高手"};
    if (m_aiLevelLabel == nullptr) { return; }
    int lv = GameConfig::getInstance()->m_iAiLevel;
    if (lv < 0 || lv > 2) { lv = 1; }
    m_aiLevelLabel->setString(std::string("AI 强度：") + kLevels[lv]);
}

/**
 * 刷新起始积分显示文本
 */
void SettingDlg::refreshStartScoreLabel() {
    if (m_startScoreLabel == nullptr) { return; }
    m_startScoreLabel->setString(std::string("起始积分：")
            + utility::toString(GameConfig::getInstance()->m_iStartScore) + " 分");
}

/**
 * 刷新基本分显示文本
 */
void SettingDlg::refreshBaseFanLabel() {
    if (m_baseFanLabel == nullptr) { return; }
    m_baseFanLabel->setString(std::string("基本分：")
            + utility::toString(GameConfig::getInstance()->m_iBaseFan) + " 分");
}
