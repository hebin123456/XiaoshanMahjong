//
// Created by farmer on 2018/7/8.
//  游戏设置层

#ifndef COCOSTUDIO_MAHJONG_SETLAYER_H
#define COCOSTUDIO_MAHJONG_SETLAYER_H

#include "IDialog.h"

class SettingDlg : public IDialog<Layer>
{
public:
    SettingDlg();
    virtual ~SettingDlg();
    CREATE_FUNC(SettingDlg);

protected:
    virtual const char* csbName() const {return "res/SetLayer.csb";}
    virtual void onUILoaded();

private:
    static const int RULE_COUNT = 3;    //萧山规则开关数量
    Button* m_btnClose;
    ui::Slider * m_pSliderVolume;   //声音滑动条
    Label* m_ruleLabels[RULE_COUNT];    //规则开关文本（可点击切换）
    bool*  m_ruleValues[RULE_COUNT];    //规则开关对应的配置值
    Label* m_aiLevelLabel;              //AI 强度文本（点击循环切换）
    Label* m_startScoreLabel;           //起始积分文本（点击切换 50/100）
    Label* m_baseFanLabel;              //基本分（底分）文本（点击循环 1/2/4/8）

    void buildRuleSwitches();           //构建规则开关控件
    void refreshRuleLabel(int index);   //刷新单个开关的显示文本
    void refreshAiLevelLabel();         //刷新 AI 强度显示文本
    void refreshStartScoreLabel();      //刷新起始积分显示文本
    void refreshBaseFanLabel();         //刷新基本分显示文本
};

#endif //COCOSTUDIO_MAHJONG_SETLAYER_H
