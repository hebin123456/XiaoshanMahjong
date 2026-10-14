//
// Created by farmer on 2018/7/5.
//

#include "GameLogic.h"
#include <random>
#include <cstring>
#include <functional>
#include <vector>

//萧山麻将规则开关默认值（保持与上游一致的兼容行为，可由 GameConfig 覆盖）
GameLogic::RuleFlags GameLogic::s_RuleFlags = { true, false, true };

/**
 * 设置规则开关
 * @param flags
 *  由 GameConfig 注入的规则开关
 */
void GameLogic::setRuleFlags(const RuleFlags &flags) {
    s_RuleFlags = flags;
}

//本局财神（发牌后翻出的那张牌，万能牌）。
//此处仅作兜底初值：正常流程每局开始都会翻一张牌并调用 setJoker() 覆盖。
uint8_t GameLogic::s_JokerCard = WHITE_CARD;
uint8_t GameLogic::s_JokerIndex = WHITE_INDEX;

/**
 * 设置本局财神（翻出的那张牌）
 * @param cbCardData 财神牌
 */
void GameLogic::setJoker(uint8_t cbCardData) {
    s_JokerCard = cbCardData;
    s_JokerIndex = switchToCardIndex(cbCardData);
}

/**
 * 是否为本局财神牌（翻出的那张牌，万能牌）
 * @param cbCardData
 * @return
 */
bool GameLogic::isJokerCard(uint8_t cbCardData) {
    return cbCardData == s_JokerCard;
}

/**
 * 是否为本局财神牌或其替身白板
 * 萧山麻将：打出财神即为「飘」——翻出的财神牌本身、以及财神的替身白板，
 * 打出后均按飘处理（该牌不能被吃/碰/杠、不能被点炮胡）。
 * @param cbCardData
 * @return
 */
bool GameLogic::isJokerRelated(uint8_t cbCardData) {
    return cbCardData == s_JokerCard || cbCardData == WHITE_CARD;
}

//麻将牌数据
static const uint8_t s_MJCardDataArray[MAX_REPERTORY] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,                        //筒子
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,                        //筒子
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,                        //筒子
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,                        //筒子
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,                        //万子
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,                        //万子
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,                        //万子
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,                        //万子
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,                        //条子
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,                        //条子
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,                        //条子
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,                        //条子
        0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,                                    //番子
        0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,                                    //番子
        0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,                                    //番子
        0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,                                    //番子
};

/**
 * 洗牌
 * @param cbCardData
 * @param cbMaxCount
 */
void GameLogic::shuffle(uint8_t *cbCardData, uint8_t cbMaxCount) {
    std::random_device rd;
    uint8_t cbCardDataTemp[sizeof(s_MJCardDataArray)];
    memcpy(cbCardDataTemp, s_MJCardDataArray, sizeof(s_MJCardDataArray));
    uint8_t cbRandCount = 0, cbPosition = 0;
    do {
        cbPosition = static_cast<uint8_t>(rd() % (cbMaxCount - cbRandCount));
        cbCardData[cbRandCount++] = cbCardDataTemp[cbPosition];
        cbCardDataTemp[cbPosition] = cbCardDataTemp[cbMaxCount - cbRandCount];
    } while (cbRandCount < cbMaxCount);
}

/**
 * Index转成牌
 * @param cbCardIndex
 * @return
 */
uint8_t GameLogic::switchToCardData(uint8_t cbCardIndex) {
    return static_cast<uint8_t>(((cbCardIndex / 9) << 4) | (cbCardIndex % 9 + 1));;
}

/**
 * 牌转成Index
 * @param cbCardData
 * @return
 */
uint8_t GameLogic::switchToCardIndex(uint8_t cbCardData) {
    return static_cast<uint8_t>(((cbCardData & MASK_COLOR) >> 4) * 9 + (cbCardData & MASK_VALUE) - 1);
}

/**
 * Index转成牌
 * @param cbCardIndex
 *  牌的Index
 * @param cbCardData
 *  牌
 * @param bMaxCount
 *  数量
 * @return
 */
uint8_t GameLogic::switchToCardData(uint8_t *cbCardIndex, uint8_t *cbCardData, uint8_t bMaxCount) {
    uint8_t bPosition = 0;
    for (uint8_t i = 0; i < MAX_INDEX; i++) {
        if (cbCardIndex[i] != 0) {
            for (uint8_t j = 0; j < cbCardIndex[i]; j++) {
                cbCardData[bPosition++] = switchToCardData(i);
            }
        }
    }
    return bPosition;
}

/**
 * 牌转成Index
 * @param cbCardData
 *  牌
 * @param cbCardCount
 *  数量
 * @param cbCardIndex
 *  牌的Index
 * @return
 */
uint8_t GameLogic::switchToCardIndex(uint8_t *cbCardData, uint8_t cbCardCount, uint8_t *cbCardIndex) {
    //设置变量
    memset(cbCardIndex, 0, sizeof(uint8_t) * MAX_INDEX);
    //转换扑克
    for (uint8_t i = 0; i < cbCardCount; i++) {
        cbCardIndex[switchToCardIndex(cbCardData[i])]++;
    }
    return cbCardCount;
}


/**
 * 是否有效的牌
 * @param cbCardData
 * @return
 */
bool GameLogic::isValidCard(uint8_t cbCardData) {
    uint8_t cbValue = static_cast<uint8_t>(cbCardData & MASK_VALUE);
    uint8_t cbColor = static_cast<uint8_t>((cbCardData & MASK_COLOR) >> 4);
    return (((cbValue >= 1) && (cbValue <= 9) && (cbColor <= 2)) || ((cbValue >= 1) && (cbValue <= 7) && (cbColor == 3)));
}

/**
 * 移除牌
 * @param cbCardIndex
 *  牌数组
 * @param cbRemoveCard
 *  移除的牌
 * @return
 */
bool GameLogic::removeCard(uint8_t *cbCardIndex, uint8_t cbRemoveCard) {
    uint8_t cbRemoveIndex = switchToCardIndex(cbRemoveCard);     //删除扑克
    if (cbCardIndex[cbRemoveIndex] > 0) {
        cbCardIndex[cbRemoveIndex]--;
        return true;
    }
    return false;
}

/**
 * 移除牌
 * @param cbCardIndex
 *  牌数组
 * @param cbRemoveCard
 *  牌
 * @param cbRemoveCount
 *  数量
 * @return
 */
bool GameLogic::removeCard(uint8_t *cbCardIndex, uint8_t *cbRemoveCard, uint8_t cbRemoveCount) {
    //删除扑克
    for (uint8_t i = 0; i < cbRemoveCount; i++) {
        uint8_t cbRemoveIndex = switchToCardIndex(cbRemoveCard[i]);
        if (cbCardIndex[cbRemoveIndex] == 0) {
            for (uint8_t j = 0; j < i; j++) {
                cbCardIndex[switchToCardIndex(cbRemoveCard[j])]++;
            }
            return false;
        } else {
            //删除扑克
            --cbCardIndex[cbRemoveIndex];
        }
    }
    return true;
}

/**
 * 移除牌
 * @param cbCardData
 *  牌数组
 * @param cbCardCount
 *  牌数量
 * @param cbRemoveCard
 *  牌
 * @param cbRemoveCount
 *  牌数量
 * @return
 */
bool GameLogic::removeCard(uint8_t *cbCardData, uint8_t cbCardCount, uint8_t *cbRemoveCard, uint8_t cbRemoveCount) {
    //定义变量
    uint8_t cbDeleteCount = 0, cbTempCardData[MAX_COUNT];
    if (cbCardCount > sizeof(cbTempCardData)) return false;
    memcpy(cbTempCardData, cbCardData, cbCardCount * sizeof(cbCardData[0]));
    for (uint8_t i = 0; i < cbRemoveCount; i++) {
        for (uint8_t j = 0; j < cbCardCount; j++) {
            if (cbRemoveCard[i] == cbTempCardData[j]) {
                cbDeleteCount++;
                cbTempCardData[j] = 0;
                break;
            }
        }
    }
    //成功判断
    if (cbDeleteCount != cbRemoveCount) {
        return false;
    }
    //清理扑克
    uint8_t cbCardPos = 0;
    for (uint8_t i = 0; i < cbCardCount; i++) {
        if (cbTempCardData[i] != 0) {
            cbCardData[cbCardPos++] = cbTempCardData[i];
        }
    }
    return true;
}

/**
 * 删除指定的麻将
 * @param cbCardIndex
 *  麻将数组
 * @param cbRemoveCard
 *  麻将
 * @return
 */
bool GameLogic::removeAllCard(uint8_t cbCardIndex[MAX_INDEX], uint8_t cbRemoveCard) {
    uint8_t cbRemoveIndex = switchToCardIndex(cbRemoveCard);
    cbCardIndex[cbRemoveIndex] = 0;
    return true;
}

/**
 * 获取牌数量
 * @param cbCardIndex
 * @return
 */
uint8_t GameLogic::getCardCount(uint8_t *cbCardIndex) {
    //数目统计
    uint8_t cbCardCount = 0;
    for (uint8_t i = 0; i < MAX_INDEX; i++) {
        cbCardCount += cbCardIndex[i];
    }
    return cbCardCount;
}


/**
 * 获取组合的牌
 * @param cbWeaveKind
 *  组合类型
 * @param cbCenterCard
 *  中心牌
 * @param cbCardBuffer
 *  获取的牌
 * @return
 */
uint8_t GameLogic::getWeaveCard(uint8_t cbWeaveKind, uint8_t cbCenterCard, uint8_t *cbCardBuffer) {
    //组合扑克
    switch (cbWeaveKind) {
        case WIK_P:        //碰牌操作
        {
            //设置变量
            cbCardBuffer[0] = cbCenterCard;
            cbCardBuffer[1] = cbCenterCard;
            cbCardBuffer[2] = cbCenterCard;
            return 3;
        }
        case WIK_G:        //杠牌操作
        {
            //设置变量
            cbCardBuffer[0] = cbCenterCard;
            cbCardBuffer[1] = cbCenterCard;
            cbCardBuffer[2] = cbCenterCard;
            cbCardBuffer[3] = cbCenterCard;
            return 4;
        }
        default: {
        }
    }
    return 0;
}

/**
 * 检测碰
 * @param cbCardIndex
 * @param cbCurrentCard
 * @return
 */
uint8_t GameLogic::estimatePengCard(uint8_t *cbCardIndex, uint8_t cbCurrentCard) {
    //碰牌判断
    return static_cast<uint8_t>((cbCardIndex[switchToCardIndex(cbCurrentCard)] >= 2) ? WIK_P : WIK_NULL);
}

/**
 * 检测杠
 * @param cbCardIndex
 * @param cbCurrentCard
 * @return
 */
uint8_t GameLogic::estimateGangCard(uint8_t *cbCardIndex, uint8_t cbCurrentCard) {
    return static_cast<uint8_t>((cbCardIndex[switchToCardIndex(cbCurrentCard)] == 3) ? WIK_G : WIK_NULL);
}

/**
 * 分析杠
 * @param cbCardIndex
 *  手上的牌
 * @param WeaveItem
 *  碰、杠的牌
 * @param cbWeaveCount
 *  碰、杠数量
 * @param GangCardResult
 *  杠分析结果
 * @return
 */
uint8_t GameLogic::analyseGangCard(const uint8_t *cbCardIndex, tagWeaveItem *WeaveItem, uint8_t cbWeaveCount, tagGangCardResult &GangCardResult) {
    //设置变量
    uint8_t cbActionMask = WIK_NULL;
    memset(&GangCardResult, 0, sizeof(GangCardResult));
    //手上杠牌
    for (uint8_t i = 0; i < MAX_INDEX; i++) {
        if (cbCardIndex[i] == 4) {
            cbActionMask |= WIK_G;
            GangCardResult.cbPublic[GangCardResult.cbCardCount] = FALSE;
            GangCardResult.cbCardData[GangCardResult.cbCardCount++] = switchToCardData(i);
        }
    }
    //组合杠牌
    for (uint8_t i = 0; i < cbWeaveCount; i++) {
        if (WeaveItem[i].cbWeaveKind == WIK_P) {
            if (cbCardIndex[switchToCardIndex(WeaveItem[i].cbCenterCard)] == 1) {
                cbActionMask |= WIK_G;
                GangCardResult.cbPublic[GangCardResult.cbCardCount] = TRUE;
                GangCardResult.cbCardData[GangCardResult.cbCardCount++] = WeaveItem[i].cbCenterCard;
            }
        }
    }
    return cbActionMask;
}

/**
 * 分析胡牌
 * @param cbCardIndex
 *  手上的牌
 * @param WeaveItem
 *  碰、杠的牌
 * @param cbWeaveCount
 *  碰、杠数量
 * @param cbCurrentCard
 *  当前牌
 * @param huKind
 *  胡牌类型
 * @param huRight
 *  胡牌权重
 * @param huSpecial
 *  特殊类型
 * @param cbSendCardCount
 *  发牌总数
 * @param cbOutCardCount
 *  出牌总数
 * @param bGangStatus
 *  当前杠的状态（杠开）
 * @param bZimo
 *  自摸标识
 * @param bQiangGangStatus
 *  枪杆状态（枪杆）
 * @param cbFanShu
 *  牌型的番数
 * @param bCheck
 *  校验位（处理 杠先碰 在杠逻辑）
 * @return
 */
uint8_t GameLogic::analyseHuCard(const uint8_t *cbCardIndex, tagWeaveItem *WeaveItem, uint8_t cbWeaveCount, uint8_t cbCurrentCard, uint8_t &huKind, uint64_t &huRight, uint8_t &huSpecial, const uint8_t cbSendCardCount, const uint8_t cbOutCardCount, const bool bGangStatus, const bool bZimo, const bool bQiangGangStatus, uint8_t &cbFanShu, const bool bCheck) {
    //=================构造扑克开始
    uint8_t cbCardIndexTemp[MAX_INDEX];
    memcpy(cbCardIndexTemp, cbCardIndex, sizeof(cbCardIndexTemp));                //临时扑克，用来分析
    if (cbCurrentCard != 0) {cbCardIndexTemp[switchToCardIndex(cbCurrentCard)]++;}//14只牌
    //计算数目
    uint8_t cbCardCountTemp = getCardCount(cbCardIndexTemp);
    uint8_t cbCardCount = static_cast<uint8_t>(cbCardCountTemp - 1);
    //=================构造扑克结束
    //萧山麻将：归一化——本局财神牌为万能牌（单独抽出计数 jokers）；
    //白板为财神的替身，只当财神那张牌用，故并入财神索引作为普通牌
    uint8_t jokers = cbCardIndexTemp[s_JokerIndex];
    cbCardIndexTemp[s_JokerIndex] = 0;
    if (s_JokerIndex != WHITE_INDEX) {
        cbCardIndexTemp[s_JokerIndex] += cbCardIndexTemp[WHITE_INDEX];
        cbCardIndexTemp[WHITE_INDEX] = 0;
    }
    //分析扑克
    CAnalyseItemArray AnalyseItemArray;
    AnalyseItemArray.clear();
    analyseCard(cbCardIndexTemp, cbCardCountTemp, WeaveItem, cbWeaveCount, AnalyseItemArray, jokers);
    //*********************************分析胡牌方式开始**********************************************//
    //平胡
    huRight |= pingHu(cbCardIndexTemp, cbCardCountTemp, cbCardIndex, cbCardCount, WeaveItem, cbWeaveCount, AnalyseItemArray);
    //碰碰胡
    huRight |= pengPengHu(cbCardIndexTemp, cbCardCountTemp, cbCardIndex, cbCardCount, WeaveItem, cbWeaveCount, AnalyseItemArray);
    //清一色
    huRight |= qingSe(cbCardIndexTemp, cbCardCountTemp, cbCardIndex, cbCardCount, WeaveItem, cbWeaveCount, AnalyseItemArray, jokers);
    //七对
    huRight |= qiDui(cbCardIndexTemp, cbCardCountTemp, cbCardIndex, cbCardCount, WeaveItem, cbWeaveCount, AnalyseItemArray);
    //钓鱼
    huRight |= diaoYu(cbCardIndexTemp, cbCardCountTemp, cbCardIndex, cbCardCount, WeaveItem, cbWeaveCount, AnalyseItemArray);
    //萧山麻将：字一色（全部为字牌）
    huRight |= ziYiSe(cbCardIndexTemp, cbCardCountTemp, cbCardIndex, cbCardCount, WeaveItem, cbWeaveCount, AnalyseItemArray, jokers);
    //萧山麻将：全风子——手牌与组合全为字牌（真财神万能可充字牌），免拆顺/刻直接认胡，按字一色×16计（极难达成）
    {
        bool bAllFengZi = true;
        for (uint8_t i = 0; i < MAX_INDEX; i++) {
            if (cbCardIndexTemp[i] != 0 && (switchToCardData(i) & MASK_COLOR) != 0x30) { bAllFengZi = false; break; }
        }
        if (bAllFengZi) {
            for (uint8_t i = 0; i < cbWeaveCount; i++) {
                if ((WeaveItem[i].cbCenterCard & MASK_COLOR) != 0x30) { bAllFengZi = false; break; }
            }
        }
        if (bAllFengZi) {
            huRight |= CHR_ZY;
        }
    }
    //萧山麻将：爆头（有财神牌且财神牌作将）
    if (jokers > 0 || cbCardIndexTemp[s_JokerIndex] > 0) {
        for (size_t i = 0; i < AnalyseItemArray.size(); i++) {
            if (AnalyseItemArray[i].cbCardEye == s_JokerCard) {
                huRight |= CHR_BT;
                break;
            }
        }
    }
    //********************************分析胡牌类型***************************************************//
    huKind |= ziMo(huRight, bGangStatus, bZimo);            //自摸
    huKind |= gangKai(huRight, bGangStatus, bZimo);         //杠开
    huKind |= qiangGang(huRight, bQiangGangStatus, bZimo);  //枪杆
    huKind |= jiePao(huRight, bQiangGangStatus, bZimo);     //接炮

    //*********************************分析特殊牌****************************************************//
    huSpecial |= gangPai(cbCardIndexTemp);                          //有杠
    huSpecial |= danZhang(cbCardCount);                             //单张牌
    huSpecial |= diHu(cbSendCardCount, cbOutCardCount);             //地胡
    huSpecial |= tianHu(cbSendCardCount, cbOutCardCount);           //天胡
    huSpecial |= kaZhang(cbCardIndex, WeaveItem, cbWeaveCount);     //卡张判定
    //************************************************************************************************//

    //结果判断
    if (huRight != 0x0) {                                                   //胡牌
        //萧山麻将：允许捉冲（点炮胡）开关，关闭后非自摸且非抢杠不可胡
        if (!bZimo && !bQiangGangStatus && !s_RuleFlags.allowChiHu) {
            huRight = 0x0;
            return WIK_NULL;
        }
        //萧山麻将：有财神必须爆头（财神作将），手中财神牌且未形成爆头（七对、全风子除外）不可胡
        if (s_RuleFlags.jokerBaoTou && jokers > 0
            && (huRight & CHR_BT) == 0 && (huRight & CHR_QD) == 0 && (huRight & CHR_ZY) == 0) {
            huRight = 0x0;
            return WIK_NULL;
        }
        /*
        //有些地方麻将规则胡牌和番数有关，需要在此处理
        uint8_t cbFs = getHuFanShu(huRight, huKind, huSpecial);             //计算番数
        //如果bCheck为true，永远返回false，不存在枪杆
        if ((huKind & CHK_QG) == 0x0 && !bZimo) {                           //不是枪杆并且不是自摸才判定
            if (!bCheck && cbFs <= cbFanShu)                                //番数如果不大于这轮最大的番数则不可以胡牌，枪杆除外。
            {
                return WIK_NULL;
            }
        }
        cbFanShu = cbFs;                                                    //暂存番数
        if (cbFs < 0x2)                                                     //1分的平湖
        {
            if ((huKind & CHK_ZM) == 0x0 && (huKind & CHK_QG) == 0x0)       //麻将不能胡
            {
                return WIK_NULL;
            }
        }*/
        return WIK_H;
    }
    return WIK_NULL;
}

/**
 * 获取胡牌番数
 * @param huRight
 * @param huKind
 * @param huSpecial
 * @return
 */
uint8_t GameLogic::getHuFanShu(const uint64_t huRight, const uint8_t huKind, const uint8_t huSpecial) {
    //萧山麻将倍数表（可叠加，封顶 64 倍）：
    //自摸×1(基础) | 碰碰胡×2 | 七对×4 | 豪华七对×8 | 清一色/清七对×8 | 字一色×16 | 天胡/地胡×16
    //爆头(财神作将)×2 | 杠开×2
    uint8_t cbFan = 1;
    if ((huRight & CHR_PPH) != 0) cbFan = 2;                        //碰碰胡
    if ((huRight & CHR_QD) != 0) cbFan = 4;                         //七对
    if ((huRight & CHR_HQD) != 0) cbFan = 8;                        //豪华七对（七对中含四张相同）
    if ((huRight & CHR_QS) != 0) cbFan = 8;                         //清一色(清七对亦为8)
    if ((huRight & CHR_ZY) != 0) cbFan = 16;                        //字一色
    if ((huSpecial & (CHS_TH | CHS_DH)) != 0) cbFan = 16;           //天胡/地胡（存于 huSpecial）
    if ((huRight & CHR_BT) != 0) cbFan = (cbFan * 2 > 64) ? 64 : static_cast<uint8_t>(cbFan * 2);   //爆头
    if ((huKind & CHK_GK) != 0) cbFan = (cbFan * 2 > 64) ? 64 : static_cast<uint8_t>(cbFan * 2);    //杠开
    return cbFan;
}

/**
 * 获取操作权重
 * @param wUserAction
 * @return
 */
uint8_t GameLogic::getUserActionRank(uint8_t cbUserAction) {
    if ((cbUserAction & WIK_H) != 0x0) {return 3;}  //胡牌优先级
    if ((cbUserAction & WIK_G) != 0x0) {return 2;} //杠牌优先级
    if ((cbUserAction & WIK_P) != 0x0) {return 1;} //碰牌优先级
    return 0;
}

/**
 * 分析能够胡牌的数量
 * @param cbCardIndex
 * @param WeaveItem
 * @param cbWeaveCount
 * @return
 */
uint8_t GameLogic::analyseHuCardCount(const uint8_t *cbCardIndex, tagWeaveItem *WeaveItem, uint8_t cbWeaveCount) {
    uint8_t cbCount = 0;
    uint8_t cbCardIndexTemp[MAX_INDEX];
    memcpy(cbCardIndexTemp, cbCardIndex, sizeof(cbCardIndexTemp));
    for (uint8_t j = 0; j < MAX_INDEX; j++) {
        uint8_t cbCurrentCard = switchToCardData(j);
        if (analyseCanHuCard(cbCardIndexTemp, WeaveItem, cbWeaveCount, cbCurrentCard)) {
            cbCount++;
        }
    }
    return cbCount;
}

/**
 * 含财神（万能牌）时的分析扑克
 * 财神牌（翻出的那张牌）为万能牌，可顶替任意牌；白板已在调用方归一化为
 * 「财神那张牌」的普通牌。枚举「将牌 + 若干面子」的所有拆分方式，
 * 结果写入 AnalyseItemArray，供平胡/碰碰胡/清一色等番型函数复用。
 * @param cbCardIndex
 *  手上的牌（白板已并入财神索引，自然财神牌已抽出）
 * @param cbCardCount
 *  牌数量（应为 3n+2，含被抽出的财神牌）
 * @param WeaveItem
 *  已亮出的碰/杠/吃组合
 * @param cbItemCount
 *  组合数量
 * @param AnalyseItemArray
 *  分析结果
 * @param jokers
 *  手中财神牌（万能牌）数量
 */
void GameLogic::analyseCardJoker(const uint8_t *cbCardIndex, const uint8_t cbCardCount, tagWeaveItem *WeaveItem, uint8_t cbItemCount, CAnalyseItemArray &AnalyseItemArray, const uint8_t jokers) {
    const size_t kMaxResult = 64;   //拆分方式上限，避免组合爆炸
    uint8_t counts[MAX_INDEX];
    memcpy(counts, cbCardIndex, sizeof(counts));
    uint8_t sets = static_cast<uint8_t>((cbCardCount - 2) / 3);   //需要的面子数

    struct MJMeld { uint8_t kind; uint8_t centerCard; };
    std::vector<MJMeld> melds;
    uint8_t eye = 0;

    auto allZero = [&]() -> bool {
        for (uint8_t k = 0; k < MAX_INDEX; k++) { if (counts[k] > 0) return false; }
        return true;
    };
    auto firstNonZero = [&]() -> uint8_t {
        for (uint8_t k = 0; k < MAX_INDEX; k++) { if (counts[k] > 0) return k; }
        return 0xFF;
    };
    auto pushItem = [&]() {
        if (AnalyseItemArray.size() >= kMaxResult) return;
        tagAnalyseItem item;
        memset(&item, 0, sizeof(item));
        for (uint8_t i = 0; i < cbItemCount; i++) {
            item.cbWeaveKind[i] = WeaveItem[i].cbWeaveKind;
            item.cbCenterCard[i] = WeaveItem[i].cbCenterCard;
        }
        for (size_t i = 0; i < melds.size(); i++) {
            item.cbWeaveKind[cbItemCount + i] = melds[i].kind;
            item.cbCenterCard[cbItemCount + i] = melds[i].centerCard;
        }
        item.cbCardEye = eye;
        AnalyseItemArray.push_back(item);
    };

    uint8_t curJokers = jokers;
    uint8_t curSets = sets;
    std::function<void(uint8_t, uint8_t)> enumSeqChoices;
    std::function<void()> enumSetSol;

    //在 [base, base+2] 三张牌上依次选择「用真实牌 / 用财神」，穷举该顺子的取法
    enumSeqChoices = [&](uint8_t pos, uint8_t base) {
        if (pos > static_cast<uint8_t>(base + 2)) {
            melds.push_back({WIK_S, switchToCardData(static_cast<uint8_t>(base + 1))});
            curSets--;
            enumSetSol();
            curSets++;
            melds.pop_back();
            return;
        }
        if (counts[pos] > 0) {          //用真实牌
            counts[pos]--;
            enumSeqChoices(static_cast<uint8_t>(pos + 1), base);
            counts[pos]++;
        }
        if (curJokers > 0) {            //用财神顶替
            curJokers--;
            enumSeqChoices(static_cast<uint8_t>(pos + 1), base);
            curJokers++;
        }
    };

    //递归拆分剩余牌为 curSets 个面子
    enumSetSol = [&]() {
        if (curSets == 0) {
            if (allZero() && curJokers == 0) pushItem();
            return;
        }
        uint8_t i = firstNonZero();
        if (i == 0xFF) {                //只剩财神：三张财神可作任意刻子
            if (curJokers == 3u * curSets) {
                for (uint8_t s = 0; s < curSets; s++) melds.push_back({WIK_P, s_JokerCard});
                uint8_t savedSets = curSets, savedJokers = curJokers;
                curSets = 0; curJokers = 0;
                if (allZero()) pushItem();
                curSets = savedSets; curJokers = savedJokers;
                for (uint8_t s = 0; s < savedSets; s++) melds.pop_back();
            }
            return;
        }
        //方案一：以 i 做刻子（用 k 张真实牌 + (3-k) 张财神）
        uint8_t maxK = counts[i] > 3 ? 3 : counts[i];
        for (uint8_t k = 1; k <= maxK; k++) {
            uint8_t needJ = static_cast<uint8_t>(3 - k);
            if (curJokers < needJ) continue;
            counts[i] -= k; curJokers -= needJ; curSets--;
            melds.push_back({WIK_P, switchToCardData(i)});
            enumSetSol();
            melds.pop_back(); curSets++; curJokers += needJ; counts[i] += k;
        }
        //方案二：以 i 参与顺子，顺子起点可能前移 0/1/2 位（前移位由财神顶替）
        for (int off = 0; off <= 2; off++) {
            int base = static_cast<int>(i) - off;
            if (base < 0 || base >= 27) continue;                       //顺子仅限数牌
            if ((base / 9) != (static_cast<int>(i) / 9)) continue;      //必须同花色
            if ((base % 9) > 6) continue;                               //不能跨花色
            enumSeqChoices(static_cast<uint8_t>(base), static_cast<uint8_t>(base));
        }
    };

    auto tryPair = [&]() {
        curSets = sets;
        melds.clear();
        enumSetSol();
    };

    //枚举将牌：真实对子 / 真实牌+财神 / 财神+财神
    for (uint8_t i = 0; i < MAX_INDEX; i++) {
        if (counts[i] >= 2) {
            counts[i] -= 2; eye = switchToCardData(i);
            curJokers = jokers;
            tryPair();
            counts[i] += 2;
        }
    }
    for (uint8_t i = 0; i < MAX_INDEX; i++) {
        if (counts[i] >= 1 && jokers >= 1) {
            counts[i] -= 1; eye = s_JokerCard;   //财神参与作将 -> 爆头
            curJokers = static_cast<uint8_t>(jokers - 1);
            tryPair();
            counts[i] += 1;
        }
    }
    if (jokers >= 2) {
        eye = s_JokerCard;
        curJokers = static_cast<uint8_t>(jokers - 2);
        tryPair();
    }
}

bool GameLogic::analyseCard(const uint8_t *cbCardIndex, const uint8_t cbCardCount, tagWeaveItem *WeaveItem, uint8_t cbItemCount, CAnalyseItemArray &AnalyseItemArray, const uint8_t jokers) {
    //效验数目
    if ((cbCardCount < 2) || (cbCardCount > MAX_COUNT) || ((cbCardCount - 2) % 3 != 0)) return false;
    //萧山麻将：手中含财神牌（万能牌）时，走万能牌拆分逻辑
    if (jokers > 0) {
        analyseCardJoker(cbCardIndex, cbCardCount, WeaveItem, cbItemCount, AnalyseItemArray, jokers);
        return (AnalyseItemArray.size() > 0);
    }
    //变量定义
    uint8_t cbKindItemCount = 0;
    tagKindItem KindItem[2 * MAX_INDEX];
    memset(KindItem, 0, sizeof(KindItem));
    //需求判断
    uint8_t cbLessKindItem = static_cast<uint8_t>((cbCardCount - 2) / 3);
    //单吊判断
    if (cbLessKindItem == 0) {
        //牌眼判断
        for (uint8_t i = 0; i < MAX_INDEX; i++) {
            if (cbCardIndex[i] == 2) {
                //变量定义
                tagAnalyseItem AnalyseItem;
                memset(&AnalyseItem, 0, sizeof(AnalyseItem));
                //设置结果
                for (uint8_t j = 0; j < cbItemCount; j++) {
                    AnalyseItem.cbWeaveKind[j] = WeaveItem[j].cbWeaveKind;
                    AnalyseItem.cbCenterCard[j] = WeaveItem[j].cbCenterCard;
                }
                AnalyseItem.cbCardEye = switchToCardData(i);
                AnalyseItemArray.push_back(AnalyseItem);            //插入结果
                return true;
            }
        }
        return false;
    }
    //拆分分析
    if (cbCardCount >= 3) {
        for (uint8_t i = 0; i < MAX_INDEX; i++) {
            //同牌判断
            if (cbCardIndex[i] >= 3) {
                KindItem[cbKindItemCount].cbCenterCard = switchToCardData(i);
                KindItem[cbKindItemCount].cbCardIndex[0] = i;
                KindItem[cbKindItemCount].cbCardIndex[1] = i;
                KindItem[cbKindItemCount].cbCardIndex[2] = i;
                KindItem[cbKindItemCount++].cbWeaveKind = WIK_P;
            }
            //连牌判断
            if ((i < (MAX_INDEX - 2 - 7)) && (cbCardIndex[i] > 0) && ((i % 9) < 7)) {
                for (uint8_t j = 1; j <= cbCardIndex[i]; j++) {
                    if ((cbCardIndex[i + 1] >= j) && (cbCardIndex[i + 2] >= j)) {
                        KindItem[cbKindItemCount].cbCenterCard = switchToCardData(static_cast<uint8_t>(i + 1));
                        KindItem[cbKindItemCount].cbCardIndex[0] = i;
                        KindItem[cbKindItemCount].cbCardIndex[1] = static_cast<uint8_t>(i + 1);
                        KindItem[cbKindItemCount].cbCardIndex[2] = static_cast<uint8_t>(i + 2);
                        KindItem[cbKindItemCount++].cbWeaveKind = WIK_S;
                    }
                }
            }
        }
    }

    //组合分析
    if (cbKindItemCount >= cbLessKindItem) {
        //变量定义
        uint8_t cbCardIndexTemp[MAX_INDEX];
        memset(cbCardIndexTemp, 0, sizeof(cbCardIndexTemp));
        //变量定义
        uint8_t cbIndex[MAX_WEAVE] = {0, 1, 2, 3};
        tagKindItem *pKindItem[MAX_WEAVE];
        memset(&pKindItem, 0, sizeof(pKindItem));
        //开始组合
        do {
            //设置变量
            memcpy(cbCardIndexTemp, cbCardIndex, sizeof(cbCardIndexTemp));
            for (uint8_t i = 0; i < cbLessKindItem; i++) {
                pKindItem[i] = &KindItem[cbIndex[i]];
            }
            //数量判断
            bool bEnoughCard = true;
            for (uint8_t i = 0; i < cbLessKindItem * 3; i++) {
                //存在判断
                uint8_t cbTempCardIndex = pKindItem[i / 3]->cbCardIndex[i % 3];
                if (cbCardIndexTemp[cbTempCardIndex] == 0) {
                    bEnoughCard = false;
                    break;
                } else
                    cbCardIndexTemp[cbTempCardIndex]--;
            }


            if (bEnoughCard) {            //胡牌判断
                uint8_t cbCardEye = 0;    //牌眼判断
                //是否继续
                for (uint8_t i = 0; i < MAX_INDEX; i++) {
                    if (cbCardIndexTemp[i] == 2) {
                        //眼牌数据
                        cbCardEye = switchToCardData(i);
                        //是否继续
                        break;
                    }
                }

                //组合类型
                if (cbCardEye != 0) {
                    //变量定义
                    tagAnalyseItem AnalyseItem;
                    memset(&AnalyseItem, 0, sizeof(AnalyseItem));
                    //设置组合
                    for (uint8_t i = 0; i < cbItemCount; i++) {
                        AnalyseItem.cbWeaveKind[i] = WeaveItem[i].cbWeaveKind;
                        AnalyseItem.cbCenterCard[i] = WeaveItem[i].cbCenterCard;
                    }
                    //设置牌型
                    for (uint8_t i = 0; i < cbLessKindItem; i++) {
                        AnalyseItem.cbWeaveKind[i + cbItemCount] = pKindItem[i]->cbWeaveKind;
                        AnalyseItem.cbCenterCard[i + cbItemCount] = pKindItem[i]->cbCenterCard;
                    }
                    AnalyseItem.cbCardEye = cbCardEye;      //设置牌眼
                    AnalyseItemArray.push_back(AnalyseItem);//插入结果
                }
            }

            //设置索引
            if (cbIndex[cbLessKindItem - 1] == (cbKindItemCount - 1)) {
                uint8_t i = static_cast<uint8_t>(cbLessKindItem - 1);
                for (; i > 0; i--) {
                    if ((cbIndex[i - 1] + 1) != cbIndex[i]) {
                        uint8_t cbNewIndex = cbIndex[i - 1];
                        for (uint8_t j = static_cast<uint8_t>(i - 1); j < cbLessKindItem; j++)
                            cbIndex[j] = static_cast<uint8_t>(cbNewIndex + j - i + 2);
                        break;
                    }
                }

                if (i == 0)
                    break;

            } else
                cbIndex[cbLessKindItem - 1]++;

        } while (true);

    }
    return (AnalyseItemArray.size() > 0);
}

bool GameLogic::analyseTingCard(const uint8_t *cbCardIndex, tagWeaveItem *WeaveItem, uint8_t cbWeaveCount) {
    uint8_t cbCardIndexTemp[MAX_INDEX];
    memcpy(cbCardIndexTemp, cbCardIndex, sizeof(cbCardIndexTemp));
    for (uint8_t i = 0; i < MAX_INDEX; i++) {
        if (cbCardIndexTemp[i] == 0) continue;//空牌过滤
        cbCardIndexTemp[i]--;                  //假设出掉的牌
        for (uint8_t j = 0; j < MAX_INDEX; j++) {
            uint8_t cbCurrentCard = switchToCardData(j);
            if (analyseCanHuCard(cbCardIndexTemp, WeaveItem, cbWeaveCount, cbCurrentCard)) {
                return true;
            }
        }
        cbCardIndexTemp[i]++;                 //还原假设的牌
    }
    return false;
}

/**
 * 判断是否能胡牌
 * @param cbCardIndex
 * @param WeaveItem
 * @param cbWeaveCount
 * @param cbCurrentCard
 * @return
 */
bool GameLogic::analyseCanHuCard(const uint8_t *cbCardIndex, tagWeaveItem *WeaveItem, uint8_t cbWeaveCount, uint8_t cbCurrentCard) {
    //=================构造扑克开始
    uint8_t cbCardIndexTemp[MAX_INDEX];
    memcpy(cbCardIndexTemp, cbCardIndex, sizeof(cbCardIndexTemp));                //临时扑克，用来分析
    if (cbCurrentCard != 0) {cbCardIndexTemp[switchToCardIndex(cbCurrentCard)]++;}//14只牌
    //计算数目
    uint8_t cbCardCountTemp = getCardCount(cbCardIndexTemp);
    uint8_t cbCardCount = static_cast<uint8_t>(cbCardCountTemp - 1);
    //萧山麻将：归一化（财神牌抽出为万能牌，白板并入财神索引作为普通牌）
    uint8_t jokers = cbCardIndexTemp[s_JokerIndex];
    cbCardIndexTemp[s_JokerIndex] = 0;
    if (s_JokerIndex != WHITE_INDEX) {
        cbCardIndexTemp[s_JokerIndex] += cbCardIndexTemp[WHITE_INDEX];
        cbCardIndexTemp[WHITE_INDEX] = 0;
    }
    //分析扑克
    CAnalyseItemArray AnalyseItemArray;
    AnalyseItemArray.clear();
    analyseCard(cbCardIndexTemp, cbCardCountTemp, WeaveItem, cbWeaveCount, AnalyseItemArray, jokers);
    //是否能胡牌
    return canHu(cbCardIndexTemp, cbCardCountTemp, cbCardIndex, cbCardCount, WeaveItem, cbWeaveCount, AnalyseItemArray, jokers);
}

/**
 * 分析听牌结果
 * @param cbCardIndex
 * @param WeaveItem
 * @param cbWeaveCount
 * @param tingResult
 * @return
 */
bool GameLogic::analyseTingCardResult(const uint8_t *cbCardIndex, tagWeaveItem *WeaveItem, uint8_t cbWeaveCount, tagTingResult &tingResult) {
    memset(&tingResult, 0, sizeof(tagTingResult));
    uint8_t cbCardIndexTemp[MAX_INDEX];
    memcpy(cbCardIndexTemp, cbCardIndex, sizeof(cbCardIndexTemp));
    for (uint8_t j = 0; j < MAX_INDEX; j++) {
        uint8_t cbCurrentCard = switchToCardData(j);
        if (analyseCanHuCard(cbCardIndexTemp, WeaveItem, cbWeaveCount, cbCurrentCard)) {
            tingResult.cbTingCard[tingResult.cbTingCount++] = cbCurrentCard;
        }
    }
    return tingResult.cbTingCount > 0;
}

/**
 * 判断是否能胡
 * @param cbCardIndexTemp
 * @param cbCardCountTemp
 * @param cbCardIndex
 * @param cbCardCount
 * @param WeaveItem
 * @param cbWeaveCount
 * @param AnalyseItemArray
 * @return
 */
bool GameLogic::canHu(const uint8_t cbCardIndexTemp[MAX_INDEX], const uint8_t cbCardCountTemp, const uint8_t cbCardIndex[MAX_INDEX], const uint8_t cbCardCount, tagWeaveItem WeaveItem[], uint8_t cbWeaveCount, CAnalyseItemArray &AnalyseItemArray, const uint8_t jokers) {
    if (cbWeaveCount == 0) {    //计算七对，	//不存在碰、杠
        if (jokers == 0) {
            uint8_t cbDuiCount = 0;
            for (uint8_t i = 0; i < MAX_INDEX; i++) {
                if (cbCardIndexTemp[i] == 2) //牌的数量存在奇数就是不是七对，否则是
                {
                    cbDuiCount++;
                }
            }
            if (cbDuiCount == 7) {
                return true;
            }
        } else if (cbCardCountTemp == 14) {
            //含财神的七对：财神补齐所有单张，剩余财神两两成对
            uint8_t cbOddCount = 0;
            for (uint8_t i = 0; i < MAX_INDEX; i++) {
                if ((cbCardIndexTemp[i] & 1) != 0) cbOddCount++;
            }
            if (cbOddCount <= jokers && ((jokers - cbOddCount) % 2 == 0)) {
                return true;
            }
        }
    }
    return AnalyseItemArray.size() > 0;
}


/**
 * 平湖判定
 * @param cbCardIndexTemp
 * @param cbCardCountTemp
 * @param cbCardIndex
 * @param cbCardCount
 * @param WeaveItem
 * @param cbWeaveCount
 * @param AnalyseItemArray
 * @return
 */
uint64_t GameLogic::pingHu(const uint8_t *cbCardIndexTemp, const uint8_t cbCardCountTemp, const uint8_t *cbCardIndex, const uint8_t cbCardCount, tagWeaveItem *WeaveItem, uint8_t cbWeaveCount, CAnalyseItemArray &AnalyseItemArray) {
    if (AnalyseItemArray.size() > 0) {
        for (uint8_t i = 0; i < AnalyseItemArray.size(); i++) {
            bool bLianCard = false, bPengCard = false;
            tagAnalyseItem *pAnalyseItem = &AnalyseItemArray[i];
            for (uint8_t j = 0; j < sizeof(pAnalyseItem->cbWeaveKind); j++) {
                uint8_t cbWeaveKind = pAnalyseItem->cbWeaveKind[j];
                bPengCard = ((cbWeaveKind & (WIK_G | WIK_P)) != 0) ? true : bPengCard;
                bLianCard = ((cbWeaveKind & (WIK_S)) != 0) ? true : bLianCard;
            }
            if (bLianCard == true) {//平胡
                return CHR_PH;
            }
        }
    }
    return CHR_NULL;
}

/**
 * 清一色判断
 * @param cbCardIndexTemp
 * @param cbCardCountTemp
 * @param cbCardIndex
 * @param cbCardCount
 * @param WeaveItem
 * @param cbWeaveCount
 * @param AnalyseItemArray
 * @return
 */
uint64_t GameLogic::qingSe(const uint8_t *cbCardIndexTemp, const uint8_t cbCardCountTemp, const uint8_t *cbCardIndex, const uint8_t cbCardCount, tagWeaveItem *WeaveItem, uint8_t cbWeaveCount, CAnalyseItemArray &AnalyseItemArray, const uint8_t jokers) {
    uint8_t cbCardColor = 0xFF;
    //萧山麻将：归一化后自然财神牌已抽出（万能），白板已并入财神索引作为普通牌，故此处直接按花色判定
    for (uint8_t i = 0; i < MAX_INDEX; i++) {
        if (cbCardIndexTemp[i] != 0) {
            uint8_t cbTempCardColor = static_cast<uint8_t>(switchToCardData(i) & MASK_COLOR);

            if (cbCardColor == 0xFF) {//花色判断
                cbCardColor = cbTempCardColor;
            }
            if (cbTempCardColor != cbCardColor) {
                return CHR_NULL;
            }
        }
    }
    //组合判断
    for (uint8_t i = 0; i < cbWeaveCount; i++) {
        uint8_t cbCenterCard = WeaveItem[i].cbCenterCard;
        if ((cbCenterCard & MASK_COLOR) != cbCardColor) return CHR_NULL;
    }
    //对号判定
    if (canHu(cbCardIndexTemp, cbCardCountTemp, cbCardIndex, cbCardCount, WeaveItem, cbWeaveCount, AnalyseItemArray, jokers)) {
        return CHR_QS;
    }
    return CHR_NULL;
}

/**
 * 字一色判定（全部由字牌组成；财神白板本身即为字牌，故不影响判定）
 * @param cbCardIndexTemp
 * @param cbCardCountTemp
 * @param cbCardIndex
 * @param cbCardCount
 * @param WeaveItem
 * @param cbWeaveCount
 * @param AnalyseItemArray
 * @return
 */
uint64_t GameLogic::ziYiSe(const uint8_t *cbCardIndexTemp, const uint8_t cbCardCountTemp, const uint8_t *cbCardIndex, const uint8_t cbCardCount, tagWeaveItem *WeaveItem, uint8_t cbWeaveCount, CAnalyseItemArray &AnalyseItemArray, const uint8_t jokers) {
    //手上牌必须全为字牌（花色 0x3）
    for (uint8_t i = 0; i < MAX_INDEX; i++) {
        if (cbCardIndexTemp[i] != 0) {
            if ((switchToCardData(i) & MASK_COLOR) != 0x30) { return CHR_NULL; }
        }
    }
    //组合牌必须全为字牌
    for (uint8_t i = 0; i < cbWeaveCount; i++) {
        if ((WeaveItem[i].cbCenterCard & MASK_COLOR) != 0x30) return CHR_NULL;
    }
    //对号判定
    if (canHu(cbCardIndexTemp, cbCardCountTemp, cbCardIndex, cbCardCount, WeaveItem, cbWeaveCount, AnalyseItemArray, jokers)) {
        return CHR_ZY;
    }
    return CHR_NULL;
}

/**
 * 碰碰胡判定
 * @param cbCardIndexTemp
 * @param cbCardCountTemp
 * @param cbCardIndex
 * @param cbCardCount
 * @param WeaveItem
 * @param cbWeaveCount
 * @param AnalyseItemArray
 * @return
 */
uint64_t GameLogic::pengPengHu(const uint8_t *cbCardIndexTemp, const uint8_t cbCardCountTemp, const uint8_t *cbCardIndex, const uint8_t cbCardCount, tagWeaveItem *WeaveItem, uint8_t cbWeaveCount, CAnalyseItemArray &AnalyseItemArray) {
    //胡牌分析
    if (AnalyseItemArray.size() > 0) {
        //牌型分析
        for (uint8_t i = 0; i < AnalyseItemArray.size(); i++) {
            //变量定义
            bool bLianCard = false, bPengCard = false;
            tagAnalyseItem *pAnalyseItem = &AnalyseItemArray[i];
            //牌型分析
            for (uint8_t j = 0; j < sizeof(pAnalyseItem->cbWeaveKind); j++) {
                uint8_t cbWeaveKind = pAnalyseItem->cbWeaveKind[j];
                bPengCard = ((cbWeaveKind & (WIK_G | WIK_P)) != 0) ? true : bPengCard;
                bLianCard = ((cbWeaveKind & (WIK_S)) != 0) ? true : bLianCard;
            }
            //碰碰胡
            if (!bLianCard && bPengCard) {
                return CHR_PPH;
            }
        }
    }
    return CHR_NULL;

}

/**
 * 七对判断
 * @param cbCardIndexTemp
 * @param cbCardCountTemp
 * @param cbCardIndex
 * @param cbCardCount
 * @param WeaveItem
 * @param cbWeaveCount
 * @param AnalyseItemArray
 * @return
 */
uint64_t GameLogic::qiDui(const uint8_t *cbCardIndexTemp, const uint8_t cbCardCountTemp, const uint8_t *cbCardIndex, const uint8_t cbCardCount, tagWeaveItem *WeaveItem, uint8_t cbWeaveCount, CAnalyseItemArray &AnalyseItemArray) {
    //不存在碰、杠
    if (cbWeaveCount > 0) return CHR_NULL;
    bool bHasFour = false;      //豪华七对：七对中含四张相同的牌
    for (uint8_t i = 0; i < MAX_INDEX; i++) {
        if (cbCardIndexTemp[i] == 1 || cbCardIndexTemp[i] == 3) //牌的数量存在奇数就是不是七对，否则是
        {
            return CHR_NULL;
        }
        if (cbCardIndexTemp[i] == 4) { bHasFour = true; }
    }
    return bHasFour ? (CHR_QD | CHR_HQD) : CHR_QD;
}

/**
 * 单吊判断
 * @param cbCardIndexTemp
 * @param cbCardCountTemp
 * @param cbCardIndex
 * @param cbCardCount
 * @param WeaveItem
 * @param cbWeaveCount
 * @param AnalyseItemArray
 * @return
 */
uint64_t GameLogic::diaoYu(const uint8_t *cbCardIndexTemp, const uint8_t cbCardCountTemp, const uint8_t *cbCardIndex, const uint8_t cbCardCount, tagWeaveItem *WeaveItem, uint8_t cbWeaveCount, CAnalyseItemArray &AnalyseItemArray) {
    //碰碰胡
    if (pengPengHu(cbCardIndexTemp, cbCardCountTemp, cbCardIndex, cbCardCount, WeaveItem, cbWeaveCount, AnalyseItemArray) == CHR_PPH
            && (cbCardCountTemp == 2)) {
        return CHR_DY;
    }
    return CHR_NULL;
}

/**
 * 手上有杠判断
 * @param cbCardIndex
 * @return
 */
uint8_t GameLogic::gangPai(const uint8_t *cbCardIndex) {
    uint8_t cbCardColor = 0xFF;
    for (uint8_t i = 0; i < MAX_INDEX; i++) {
        if (cbCardIndex[i] == 4) {
            return CHS_GP;
        }
    }
    return CHS_NULL;
}

/**
 * 单张判断
 * @param cbCardCount
 * @return
 */
uint8_t GameLogic::danZhang(const uint8_t cbCardCount) {
    if (cbCardCount == 2) {
        return CHS_DZ;
    }
    return CHS_NULL;
}

/**
 * 天湖判断
 * @param m_cbSendCardCount
 * @param m_cbOutCardCount
 * @return
 */
uint8_t GameLogic::tianHu(const uint8_t m_cbSendCardCount, const uint8_t m_cbOutCardCount) {
    if ((m_cbSendCardCount == 1) && (m_cbOutCardCount == 0)) {
        return CHS_TH;
    }
    return CHS_NULL;
}

/**
 * 地胡判断
 * @param m_cbSendCardCount
 * @param m_cbOutCardCount
 * @return
 */
uint8_t GameLogic::diHu(const uint8_t m_cbSendCardCount, const uint8_t m_cbOutCardCount) {
    if ((m_cbSendCardCount == 1) && (m_cbOutCardCount == 1)) {
        return CHS_DH;
    }
    return CHS_NULL;
}

/**
 * 卡张判断
 * @param cbCardIndex
 * @param WeaveItem
 * @param cbWeaveCount
 * @return
 */
uint8_t GameLogic::kaZhang(const uint8_t *cbCardIndex, tagWeaveItem *WeaveItem, uint8_t cbWeaveCount) {
    if (analyseHuCardCount(cbCardIndex, WeaveItem, cbWeaveCount) == 1) {
        return CHS_KZ;
    }
    return CHS_NULL;
}

/**
 * 自摸判断
 * @param llHuRight
 * @param bGangStatus
 * @param bZimo
 * @return
 */
uint8_t GameLogic::ziMo(uint64_t llHuRight, const bool bGangStatus, const bool bZimo) {
    if ((llHuRight != 0x0) && bZimo == true && bGangStatus == false) {
        return CHK_ZM;
    }
    return CHK_NULL;
}

/**
 * 杠开判断
 * @param llHuRight
 * @param bGangStatus
 * @param bZimo
 * @return
 */
uint8_t GameLogic::gangKai(uint64_t llHuRight, const bool bGangStatus, const bool bZimo) {
    if ((llHuRight != 0x0) && bZimo == true && bGangStatus == true) {
        return CHK_GK;
    }
    return CHK_NULL;
}

/**
 * 枪杆判断
 * @param llHuRight
 * @param bGangStatus
 * @param bZimo
 * @return
 */
uint8_t GameLogic::qiangGang(uint64_t llHuRight, const bool bGangStatus, const bool bZimo) {
    if ((llHuRight != 0x0) && bZimo == false && bGangStatus == true) {
        return CHK_QG;
    }
    return CHK_NULL;
}

/**
 * 接炮判断
 * @param llHuRight
 * @param bGangStatus
 * @param bZimo
 * @return
 */
uint8_t GameLogic::jiePao(uint64_t llHuRight, const bool bGangStatus, const bool bZimo) {
    if ((llHuRight != 0x0) && bZimo == false && bGangStatus == false) {
        return CHK_JP;
    }
    return CHK_NULL;
}
