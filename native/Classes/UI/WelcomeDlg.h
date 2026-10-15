#ifndef __COCOSTUDIO_MAHJONG__WelcomeDlg__
#define __COCOSTUDIO_MAHJONG__WelcomeDlg__

#include "IDialog.h"

class WelcomeDlg : public IDialog<Layer>
{
public:
    WelcomeDlg();
    virtual ~WelcomeDlg();
    CREATE_FUNC(WelcomeDlg);

protected:
    virtual const char* csbName() const {return "res/HelloLayer.csb";}
	virtual void onUILoaded();

private:
    void showSexChoice();      //开局前选择打牌语音性别（男声/女声）

private:
    Button* m_btnLogon;
    Node* m_pSexChoiceLayer;   //语音性别选择浮层（null=未弹出）
};


#endif /* defined(__COCOSTUDIO_MAHJONG__WelcomeDlg__) */
