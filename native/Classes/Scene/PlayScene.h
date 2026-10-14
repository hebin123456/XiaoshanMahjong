
#ifndef __COCOSTUDIO_MAHJONG__PlayScene__
#define __COCOSTUDIO_MAHJONG__PlayScene__

#include "BaseScene.h"
#include "IDialog.h"

class PlayScene : public BaseScene
{
public:
    PlayScene();
    virtual ~PlayScene();

    virtual void onEnter();
    virtual void onExit();
    virtual bool init();

    CREATE_FUNC(PlayScene);

protected:
    virtual void keyBackClicked();
};

#endif /* defined(__COCOSTUDIO_MAHJONG__PlayScene__) */
