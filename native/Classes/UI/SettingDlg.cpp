//
// Created by farmer on 2018/7/8.
//

#include "SettingDlg.h"
#include "GameConfig.h"
#include "Utility.h"
#include "GameEngine.h"

using namespace std;

SettingDlg *SettingDlg::create(bool bInGame) {
    SettingDlg *pRet = new(std::nothrow) SettingDlg(bInGame);
    if (pRet && pRet->init()) {
        pRet->autorelease();
        return pRet;
    }
    delete pRet;
    return nullptr;
}

SettingDlg::SettingDlg(bool bInGame) : IDialog(), m_bInGame(bInGame)
{
    RegDialogCtrl("Button_Close", m_btnClose);
    RegDialogCtrl("Slider_Volume", m_pSliderVolume);
    for (int i = 0; i < RULE_COUNT; i++) { m_ruleLabels[i] = nullptr; m_ruleValues[i] = nullptr; }
    m_aiLevelLabel = nullptr;
    m_startScoreLabel = nullptr;
    m_baseFanLabel = nullptr;
    m_fanCapLabel = nullptr;
    m_pInfoEntryLabel = nullptr;
    m_pInfoPanel = nullptr;
}
SettingDlg::~SettingDlg(){
}

void SettingDlg::onUILoaded() {
    //折叠屏适配：弹出动画按适配比例缩放；半透明罩铺满全屏
    LayerColor *pMask = LayerColor::create(Color4B(0, 0, 0, 200));
    this->addChild(pMask, 0);
    pMask->setContentSize(Director::getInstance()->getVisibleSize());
    pMask->setPosition(Director::getInstance()->getVisibleOrigin());
    m_rootNode->setScale(m_fFitScale * 0.5f);
    m_rootNode->runAction(EaseElasticOut::create(ScaleTo::create(0.5f, m_fFitScale, m_fFitScale))); //动画
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
    //主页模式：完整规则设置；局内模式：仅「！本局信息」（局内不允许改规则）
    if (m_bInGame) {
        buildInGameInfo();
    } else {
        buildRuleSwitches();
    }
}

/**
 * 局内模式：音量滑块下方加「！本局信息」入口，点击弹出本局倍数/基本分等信息面板
 */
void SettingDlg::buildInGameInfo() {
    //以音量滑块为锚点，入口排在滑块正下方
    Vec2 anchor = m_rootNode->convertToNodeSpace(
            m_pSliderVolume->getParent()->convertToWorldSpace(m_pSliderVolume->getPosition()));

    GameConfig *cfg = GameConfig::getInstance();
    GameEngine *pEngine = GameEngine::GetGameEngine();

    //「！本局信息」入口（金色，点击展开/收起面板）
    m_pInfoEntryLabel = Label::createWithTTF("！本局信息", "DFYuanW7-GB2312.ttf", 28);
    m_pInfoEntryLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
    m_pInfoEntryLabel->setPosition(Vec2(anchor.x, anchor.y - 80.0f));
    m_pInfoEntryLabel->setColor(Color3B(255, 235, 130));
    m_rootNode->addChild(m_pInfoEntryLabel, 10);

    //信息面板（默认隐藏，居中弹出于设置面板之上）
    const Size cs = m_rootNode->getContentSize();
    const Size panelSize(640.0f, 470.0f);
    m_pInfoPanel = LayerColor::create(Color4B(15, 28, 22, 240), panelSize.width, panelSize.height);
    m_pInfoPanel->setIgnoreAnchorPointForPosition(false);
    m_pInfoPanel->setAnchorPoint(Vec2(0.5f, 0.5f));
    m_pInfoPanel->setPosition(Vec2(cs.width * 0.5f, cs.height * 0.5f));
    m_pInfoPanel->setVisible(false);
    m_rootNode->addChild(m_pInfoPanel, 30);

    //面板内容
    auto addLine = [this](const std::string &text, float y, const Color3B &color, float fontSize = 26) {
        Label *label = Label::createWithTTF(text, "DFYuanW7-GB2312.ttf", fontSize);
        label->setAnchorPoint(Vec2(0.5f, 0.5f));
        label->setPosition(Vec2(m_pInfoPanel->getContentSize().width * 0.5f, y));
        label->setColor(color);
        m_pInfoPanel->addChild(label);
        return label;
    };
    addLine("—— 本局信息 ——", panelSize.height - 40.0f, Color3B(255, 235, 130), 30);

    uint8_t cbStreak = pEngine->getBankerStreak();
    if (cbStreak < 1) { cbStreak = 1; }   //开局掷骰前连庄数可能还是0，按首庄展示
    uint8_t cbMult = pEngine->getBankerStreakMult();
    std::string strStreak = (cbStreak <= 1) ? std::string("首庄")
            : (utility::toString((int) cbStreak) + " 连庄");
    addLine("本局倍数：×" + utility::toString((int) cbMult) + "（庄家 " + strStreak + "）",
            panelSize.height - 100.0f, Color3B(255, 210, 90));
    addLine("基本分（底分）：" + utility::toString(cfg->m_iBaseFan) + " 分",
            panelSize.height - 145.0f, Color3B(255, 255, 255));
    addLine("番数封顶：" + utility::toString(cfg->m_iFanCap) + " 番",
            panelSize.height - 190.0f, Color3B(255, 255, 255));
    static const char *kRuleTitles[RULE_COUNT] = {
            "允许捉冲(点炮胡)",
            "有财神必须爆头",
            "允许吃三摊(承包)",
    };
    const bool rulesOn[RULE_COUNT] = { cfg->m_bAllowChiHu, cfg->m_bJokerBaoTou, cfg->m_bAllowChiSanTan };
    for (int i = 0; i < RULE_COUNT; i++) {
        addLine(std::string(kRuleTitles[i]) + "：" + (rulesOn[i] ? "开" : "关"),
                panelSize.height - 235.0f - i * 45.0f,
                rulesOn[i] ? Color3B(120, 255, 120) : Color3B(190, 190, 190));
    }
    static const char *kLevels[3] = {"入门", "普通", "高手"};
    int iAiLevel = cfg->m_iAiLevel;
    if (iAiLevel < 0 || iAiLevel > 2) { iAiLevel = 1; }
    addLine(std::string("AI 强度：") + kLevels[iAiLevel],
            panelSize.height - 235.0f - RULE_COUNT * 45.0f, Color3B(255, 255, 255));

    //点击入口：展开/收起面板
    auto entryListener = EventListenerTouchOneByOne::create();
    entryListener->setSwallowTouches(true);
    entryListener->onTouchBegan = [this](Touch *touch, Event *) -> bool {
        Vec2 p = m_pInfoEntryLabel->getParent()->convertToNodeSpace(touch->getLocation());
        return m_pInfoEntryLabel->getBoundingBox().containsPoint(p);
    };
    entryListener->onTouchEnded = [this](Touch *touch, Event *) {
        Vec2 p = m_pInfoEntryLabel->getParent()->convertToNodeSpace(touch->getLocation());
        if (m_pInfoEntryLabel->getBoundingBox().containsPoint(p)) {
            bool bVisible = !m_pInfoPanel->isVisible();
            m_pInfoPanel->setVisible(bVisible);
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(entryListener, m_pInfoEntryLabel);

    //点击面板任意处收起（仅在面板可见且点击落在面板内时才拦截，避免隐藏时吞掉全屏触摸导致设置界面点不动）
    auto panelListener = EventListenerTouchOneByOne::create();
    panelListener->setSwallowTouches(true);
    panelListener->onTouchBegan = [this](Touch *touch, Event *) -> bool {
        if (m_pInfoPanel == nullptr || !m_pInfoPanel->isVisible()) { return false; }
        Vec2 p = m_pInfoPanel->getParent()->convertToNodeSpace(touch->getLocation());
        return m_pInfoPanel->getBoundingBox().containsPoint(p);
    };
    panelListener->onTouchEnded = [this](Touch *touch, Event *) {
        if (m_pInfoPanel == nullptr || !m_pInfoPanel->isVisible()) { return; }
        Vec2 p = m_pInfoPanel->getParent()->convertToNodeSpace(touch->getLocation());
        if (m_pInfoPanel->getBoundingBox().containsPoint(p)) {
            m_pInfoPanel->setVisible(false);
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(panelListener, m_pInfoPanel);
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

    //番数封顶（点击循环：16→32→64→16，默认32；连庄倍数与番型合计的上限）
    m_fanCapLabel = Label::createWithTTF("", "DFYuanW7-GB2312.ttf", 28);
    m_fanCapLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
    m_fanCapLabel->setPosition(Vec2(anchor.x, anchor.y - 80.0f - (RULE_COUNT + 3) * fLineStep));
    m_fanCapLabel->setColor(Color3B(255, 235, 130));
    m_rootNode->addChild(m_fanCapLabel, 10);
    refreshFanCapLabel();

    auto capListener = EventListenerTouchOneByOne::create();
    capListener->setSwallowTouches(true);
    capListener->onTouchBegan = [this](Touch *touch, Event *) -> bool {
        Vec2 p = m_fanCapLabel->getParent()->convertToNodeSpace(touch->getLocation());
        return m_fanCapLabel->getBoundingBox().containsPoint(p);
    };
    capListener->onTouchEnded = [this](Touch *touch, Event *) {
        Vec2 p = m_fanCapLabel->getParent()->convertToNodeSpace(touch->getLocation());
        if (m_fanCapLabel->getBoundingBox().containsPoint(p)) {
            GameConfig *cfg = GameConfig::getInstance();
            cfg->m_iFanCap = (cfg->m_iFanCap >= 64) ? 16 : (cfg->m_iFanCap * 2);  //16/32/64 循环
            refreshFanCapLabel();
            cfg->saveConfig();
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(capListener, m_fanCapLabel);
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

/**
 * 刷新番数封顶显示文本
 */
void SettingDlg::refreshFanCapLabel() {
    if (m_fanCapLabel == nullptr) { return; }
    m_fanCapLabel->setString(std::string("番数封顶：")
            + utility::toString(GameConfig::getInstance()->m_iFanCap) + " 番");
}
