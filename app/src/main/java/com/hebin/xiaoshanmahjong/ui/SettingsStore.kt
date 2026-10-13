package com.hebin.xiaoshanmahjong.ui

import android.content.Context
import com.hebin.xiaoshanmahjong.game.AiLevel
import com.hebin.xiaoshanmahjong.game.RuleSet

/** 用 SharedPreferences 记住主界面上的规则设置。 */
object SettingsStore {
    private const val FILE = "xiaoshan_settings"
    private const val KEY_RUSH = "allow_rush"
    private const val KEY_BAO = "must_bao_head"
    private const val KEY_CHI_THREE = "allow_chi_three"
    private const val KEY_AI = "ai_level"

    fun load(context: Context): RuleSet {
        val sp = context.getSharedPreferences(FILE, Context.MODE_PRIVATE)
        val defaults = RuleSet()
        return RuleSet(
            allowRush = sp.getBoolean(KEY_RUSH, defaults.allowRush),
            mustBaoHead = sp.getBoolean(KEY_BAO, defaults.mustBaoHead),
            allowChiThree = sp.getBoolean(KEY_CHI_THREE, defaults.allowChiThree),
            aiLevel = runCatching { AiLevel.valueOf(sp.getString(KEY_AI, defaults.aiLevel.name)!!) }
                .getOrDefault(defaults.aiLevel),
        )
    }

    fun save(context: Context, rules: RuleSet) {
        context.getSharedPreferences(FILE, Context.MODE_PRIVATE).edit()
            .putBoolean(KEY_RUSH, rules.allowRush)
            .putBoolean(KEY_BAO, rules.mustBaoHead)
            .putBoolean(KEY_CHI_THREE, rules.allowChiThree)
            .putString(KEY_AI, rules.aiLevel.name)
            .apply()
    }
}
