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

    for (int i = 0; i < RULE_COUNT; i++) {
        Label *label = Label::createWithTTF("", "DFYuanW7-GB2312.ttf", 28);
        label->setAnchorPoint(Vec2(0.5f, 0.5f));
        label->setPosition(Vec2(anchor.x, anchor.y - 90.0f - i * 46.0f));
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
