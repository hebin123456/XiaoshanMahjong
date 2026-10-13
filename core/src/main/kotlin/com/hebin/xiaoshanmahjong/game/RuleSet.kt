package com.hebin.xiaoshanmahjong.game

/** AI 强度。 */
enum class AiLevel { EASY, NORMAL, HARD }

/**
 * 一局规则配置（对应主界面的设置项）。
 *
 * @param allowRush 允许捉冲（别人打出的牌也可以胡）；关闭则「自摸专属」。
 * @param mustBaoHead 有财神必须爆头：手中财神至少留 1 张做将牌（爆头），多余财神可自由组合。
 * @param allowChiThree 允许吃三摊：关闭时同一家最多吃/碰/杠 2 摊；开启时第 3 摊触发承包。
 * @param aiLevel 电脑 AI 强度。
 */
data class RuleSet(
    val allowRush: Boolean = true,
    val mustBaoHead: Boolean = true,
    val allowChiThree: Boolean = false,
    val aiLevel: AiLevel = AiLevel.NORMAL,
)
