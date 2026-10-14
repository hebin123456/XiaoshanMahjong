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

    void buildRuleSwitches();           //构建规则开关控件
    void refreshRuleLabel(int index);   //刷新单个开关的显示文本
};

#endif //COCOSTUDIO_MAHJONG_SETLAYER_H
