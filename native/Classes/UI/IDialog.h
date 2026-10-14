
#ifndef COCOSTUDIO_MAHJONG_IDialog__
#define COCOSTUDIO_MAHJONG_IDialog__

#include "Common.h"
#include "DialogManager.h"

#define RegDialogCtrl(NAME, MEMBERVARIABLE)     registerCtrl(NAME, (Node**)&(this->MEMBERVARIABLE))

template<typename T = Layer>
class IDialog : public T
{
public:
	IDialog() : T()
	, m_rootNode(nullptr)
    , m_rootAction(nullptr)
    , m_canClosebyBackBtn(true)
	{
		m_ntfMgr = __NotificationCenter::getInstance();
    }

    virtual ~IDialog() {
		this->clear();
    }

    virtual bool init() {
        bool ret = T::init();
        if(ret) {
			return this->loadfile();
        }
        return ret;
    }

	bool loadfile() {
        std::string fileName = csbName();
		this->m_rootNode = CSLoader::createNode(fileName);
		if(this->m_rootNode) {
            this->m_rootAction = CSLoader::createTimeline(fileName);
			this->addChild(this->m_rootNode, 1);
			this->fitRootNode();          //折叠屏/超宽屏适配：等比缩放居中（onUILoaded前先算好m_fFitScale，供弹窗动画使用）
			this->bindNodeByName(this->m_rootNode);
			this->onUILoaded();
			return true;
		}
		return false;
	}

	virtual void onEnter() {
		T::onEnter();
		this->fitRootNode();
		//监听窗口尺寸变化（折叠屏展开/折叠、分屏拖拽），变化时重新适配布局
		this->schedule([this](float) {
			const cocos2d::Size &sz = Director::getInstance()->getVisibleSize();
			if (sz.width != m_lastFitSize.width || sz.height != m_lastFitSize.height) {
				this->fitRootNode();
			}
		}, "FitScreen");
		//this->m_ntfMgr->addObserver(this, callfuncO_selector(IDialog::onBackBtnPressed), EVENT_KEY_BACK_CLICK, NULL);
	}

	virtual void onExit() {
		this->unschedule("FitScreen");
		this->m_ntfMgr->removeAllObservers(this);
		T::onExit();
	}

    /**
     * 折叠屏/超宽屏适配：
     * 设计分辨率1280x720，实际窗口比例不定（折叠屏展开接近方形、超宽屏则更宽）。
     * 根节点等比缩放（不拉伸不裁切）后居中；延展区用深绿桌面色铺满避免露底。
     */
	void fitRootNode() {
		if (this->m_rootNode == nullptr) { return; }
		Director *pDirector = Director::getInstance();
		const cocos2d::Size sz = pDirector->getVisibleSize();
		const Vec2 origin = pDirector->getVisibleOrigin();
		m_lastFitSize = sz;
		const cocos2d::Size cs = this->m_rootNode->getContentSize();
		if (cs.width <= 0.0f || cs.height <= 0.0f) { return; }
		this->m_fFitScale = MIN(sz.width / cs.width, sz.height / cs.height);    //等比fit：窄屏（折叠展开态）缩小、宽屏不放大
		this->m_rootNode->setScale(this->m_fFitScale);
		this->m_rootNode->setPosition(origin.x + (sz.width - cs.width * this->m_fFitScale) * 0.5f,
		                              origin.y + (sz.height - cs.height * this->m_fFitScale) * 0.5f);
		if (this->m_pBgColor == nullptr) {   //延展区底色（全屏铺满，位于内容之下）
			this->m_pBgColor = LayerColor::create(Color4B(0x0E, 0x3D, 0x1F, 0xFF));
			T::addChild(this->m_pBgColor, 0);
		}
		this->m_pBgColor->setContentSize(sz);
		this->m_pBgColor->setPosition(origin);
	}

	virtual void dispose() {
		this->setVisible(false);
		this->removeFromParent();
	}

	virtual void addEventListener(const std::string& name, SEL_CallFuncO theFunction){
		this->m_ntfMgr->addObserver(this, theFunction, name, NULL);
	}

	virtual void androidKeyBackClicked() {
		DialogManager::shared()->closeDialog(this);
	}

	void onBackBtnPressed(Ref*) {
		this->androidKeyBackClicked();
	}

    CC_SYNTHESIZE_READONLY(bool, m_canClosebyBackBtn, CanClosebyBackBtn);
	
protected:
    virtual const char* csbName() const = 0;

    virtual void onUILoaded() {}

    Node* getChildByName(const char* name) const {
        Node* ret = NULL;
        if(name) {
            std::map<std::string, Node*>::const_iterator iter = this->m_allCtrl.find(name);
            if(iter != this->m_allCtrl.end()) {
                return iter->second;
            }
        }
        return ret;
    }
    Node* getChildByName(std::string name) const {
        return getChildByName(name.c_str());
    }

protected:
    bool registerCtrl(const char* name, Node** ctrl) {
        if(this->m_regCtrl.find(name) == this->m_regCtrl.end()) {
            *ctrl = nullptr;
            this->m_regCtrl.insert(std::make_pair(name, ctrl));
            return true;
        }
        return false;
    }
    bool registerCtrl(std::string name, Node** ctrl) {
        return registerCtrl(name.c_str(), ctrl);
    }

private:
	void bindNodeByName(Node* node) {
		if(node == nullptr) return;
		std::string name = node->getName();
		//log("%s", name.c_str());
		if(!name.empty()) {
			this->m_allCtrl.insert(std::make_pair(name, node));
			std::map<std::string, Node**>::iterator iter = this->m_regCtrl.find(name);
			if(iter != this->m_regCtrl.end()) {
				Node** ctrl = iter->second;
				if(*ctrl != node) {
					*ctrl = node;
				}
			}
		}
		long count = node->getChildrenCount();
		if(count > 0) {
			Vector<Node*>& childs = node->getChildren();
			for(long i = 0; i < count; ++i) {
				Node* child = childs.at(i);
				this->bindNodeByName(child);
			}
		}
	}

    void clear() {
        this->m_regCtrl.clear();
		this->m_allCtrl.clear();
    }

protected:
	__NotificationCenter* m_ntfMgr;
	Node* m_rootNode;
    ActionTimeline* m_rootAction;
	float m_fFitScale = 1.0f;        //折叠屏/超宽屏适配缩放（弹窗弹出动画的终点比例应使用此值而非1.0）
	cocos2d::Size m_lastFitSize;     //上次适配时的窗口尺寸（用于检测折叠/展开变化）
	LayerColor* m_pBgColor = nullptr; //延展区底色（铺满全屏）

private:
    std::map<std::string, Node*>  m_allCtrl;
	std::map<std::string, Node**> m_regCtrl;
};

#endif
