
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
     * 根节点等比缩放（不拉伸不裁切）后居中；两侧延展区用本层满幅背景图拉伸铺满，避免黑边。
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
		if (this->m_pBgColor == nullptr) {   //延展区底色：透明，避免出现黑边（露出本层背景图或下层场景）
			this->m_pBgColor = LayerColor::create(Color4B(0, 0, 0, 0));
			T::addChild(this->m_pBgColor, 0);
		}
		this->m_pBgColor->setContentSize(sz);
		this->m_pBgColor->setPosition(origin);
		this->fitBackground(sz);             //两侧延展区铺满可视区域
	}

	/**
	 * 铺满可视区域：把根节点内“满幅”的背景图（尺寸≈根节点的图片节点）拉伸到整个可见区域。
	 * 九宫格图片拉伸不变形；若没有满幅背景图（如居中的弹窗卡片），延展区保持透明并露出下层场景。
	 * 子类可重写以指定自己的背景节点。
	 */
	virtual void fitBackground(const cocos2d::Size &sz) {
		if (this->m_rootNode == nullptr) { return; }
		Node *pBg = this->findFullBleedBackground(this->m_rootNode);
		if (pBg == nullptr) { return; }
		const float fScale = (this->m_fFitScale > 0.0f) ? this->m_fFitScale : 1.0f;
		//保持背景图中心不变（与父节点坐标/锚点无关），再按可视区域拉伸
		const Vec2 anchor = pBg->getAnchorPoint();
		const cocos2d::Size oldSize = pBg->getContentSize();
		const Vec2 center = Vec2(pBg->getPosition().x + (0.5f - anchor.x) * oldSize.width,
		                         pBg->getPosition().y + (0.5f - anchor.y) * oldSize.height);
		pBg->setAnchorPoint(Vec2(0.5f, 0.5f));
		pBg->setPosition(center);
		if (auto pWidget = dynamic_cast<ui::Widget*>(pBg)) {
			pWidget->ignoreContentAdaptWithSize(false);   //按给定尺寸拉伸，而非按贴图原始尺寸自适应
		}
		pBg->setContentSize(cocos2d::Size(sz.width / fScale, sz.height / fScale));
	}

	//递归查找尺寸≈根节点内容尺寸的图片节点（满幅背景图）
	Node* findFullBleedBackground(Node *pNode) {
		if (pNode == nullptr) { return nullptr; }
		const cocos2d::Size cs = this->m_rootNode->getContentSize();
		Vector<Node*>& childs = pNode->getChildren();
		for (Node *pChild : childs) {
			if (pChild == this->m_pBgColor) { continue; }
			if (dynamic_cast<ui::ImageView*>(pChild) != nullptr || dynamic_cast<Sprite*>(pChild) != nullptr) {
				const cocos2d::Size s = pChild->getContentSize();
				if (s.width >= cs.width * 0.99f && s.height >= cs.height * 0.99f) {
					return pChild;
				}
			}
			Node *pFound = this->findFullBleedBackground(pChild);
			if (pFound != nullptr) { return pFound; }
		}
		return nullptr;
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
	LayerColor* m_pBgColor = nullptr; //延展区透明底层（避免黑边，位于内容之下）

private:
    std::map<std::string, Node*>  m_allCtrl;
	std::map<std::string, Node**> m_regCtrl;
};

#endif
