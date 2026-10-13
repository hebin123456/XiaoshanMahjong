package com.hebin.xiaoshanmahjong.game

/** 一组面子 / 杠。 */
data class Meld(val type: MeldType, val tiles: IntArray, val from: Int = -1) {
    override fun equals(other: Any?): Boolean =
        other is Meld && type == other.type && from == other.from && tiles.contentEquals(other.tiles)

    override fun hashCode(): Int = (type.hashCode() * 31 + from) * 31 + tiles.contentHashCode()
}

enum class MeldType {
    CHI, // 吃
    PENG, // 碰
    MING_GANG, // 明杠
    AN_GANG, // 暗杠
    BU_GANG, // 补杠
    ;

    val isGang: Boolean get() = this == MING_GANG || this == AN_GANG || this == BU_GANG
}

/** 七对判定结果。 */
data class SevenPairs(val usedJoker: Boolean)

/** 和牌计分选项。 */
data class WinOptions(
    val selfDraw: Boolean = false,
    val dealer: Boolean = false,
    val dealerStreak: Int = 0,
    /** 杠开：明杠 ×2、暗杠 ×4、补杠 ×2。 */
    val gangOpen: MeldType? = null,
    /** 本局曾打出过财神（配合自摸触发「财飘」）。 */
    val jokerDiscarded: Boolean = false,
    /** 天胡：庄家起手即胡。 */
    val heavenly: Boolean = false,
    /** 地胡：闲家第一次摸牌即胡。 */
    val earthly: Boolean = false,
)

data class WinResult(
    val win: Boolean,
    val multiplier: Int,
    val patterns: List<String>,
) {
    companion object {
        val LOSE = WinResult(false, 0, emptyList())
    }
}

// ---------------------------------------------------------------------------
// 底层求解：把剩余的牌全部拆成面子（顺子 / 刻子 / 杠），财神可当任意牌。
// ---------------------------------------------------------------------------

private fun canAllMelds(counts: IntArray, jokers: Int): Boolean {
    var i = 0
    while (i < KIND_COUNT && counts[i] == 0) i++
    if (i == KIND_COUNT) return jokers % 3 == 0 // 剩余财神自成刻子

    // 天然刻子
    if (counts[i] >= 3) {
        counts[i] -= 3
        val ok = canAllMelds(counts, jokers)
        counts[i] += 3
        if (ok) return true
    }
    // 不足三张用财神补齐成刻子
    val need = 3 - counts[i]
    if (counts[i] in 1..2 && jokers >= need) {
        val save = counts[i]
        counts[i] = 0
        val ok = canAllMelds(counts, jokers - need)
        counts[i] = save
        if (ok) return true
    }
    // 顺子（仅数牌，且起手序号 <= 7）
    if (!isHonor(i) && rankOf(i) <= 7) {
        val t1 = if (counts[i + 1] > 0) 1 else 0
        val t2 = if (counts[i + 2] > 0) 1 else 0
        val jn = 3 - 1 - t1 - t2
        if (jokers >= jn) {
            counts[i] -= 1
            counts[i + 1] -= t1
            counts[i + 2] -= t2
            val ok = canAllMelds(counts, jokers - jn)
            counts[i] += 1
            counts[i + 1] += t1
            counts[i + 2] += t2
            if (ok) return true
        }
    }
    return false
}

private fun canAllTriplets(counts: IntArray, jokers: Int): Boolean {
    var need = 0
    for (i in 0 until KIND_COUNT) {
        when (counts[i]) {
            4 -> {} // 杠，无需财神
            1 -> need += 2
            2 -> need += 1
        }
    }
    if (need > jokers) return false
    return (jokers - need) % 3 == 0
}

private fun isSevenPairsCounts(counts: IntArray, jokers: Int): SevenPairs? {
    var singles = 0
    for (i in 0 until JOKER) singles += counts[i] % 2
    if (singles <= jokers && (jokers - singles) % 2 == 0) return SevenPairs(jokers > 0)
    return null
}

private fun isAllTripletsHand(counts: IntArray, jokers: Int): Boolean {
    for (p in 0 until KIND_COUNT) {
        if (counts[p] >= 2) {
            counts[p] -= 2
            val ok = canAllTriplets(counts, jokers)
            counts[p] += 2
            if (ok) return true
        }
    }
    if (jokers >= 1) {
        for (p in 0 until KIND_COUNT) {
            if (counts[p] >= 1) {
                counts[p] -= 1
                val ok = canAllTriplets(counts, jokers - 1)
                counts[p] += 1
                if (ok) return true
            }
        }
    }
    if (jokers >= 2 && canAllTriplets(counts, jokers - 2)) return true
    return false
}

/** 将牌（雀头）中是否含财神。 */
private fun pairUsesJoker(counts: IntArray, jokers: Int): Boolean {
    if (jokers >= 1) {
        for (p in 0 until KIND_COUNT) {
            if (counts[p] >= 1) {
                counts[p] -= 1
                val ok = canAllMelds(counts, jokers - 1)
                counts[p] += 1
                if (ok) return true
            }
        }
    }
    return jokers >= 2 && canAllMelds(counts, jokers - 2)
}

// ---------------------------------------------------------------------------
// 对外接口
// ---------------------------------------------------------------------------

/**
 * 判断手牌（3n+2 张）是否和牌，财神为万能百搭。
 * @param mustBaoHead 为 true 时，若手牌含财神，则至少 1 张财神必须做将牌（爆头）。
 */
fun isWinningHand(hand: IntArray, mustBaoHead: Boolean = false): Boolean {
    if (hand.size % 3 != 2) return false
    val counts = countsOf(hand)
    val jokers = counts[JOKER]
    counts[JOKER] = 0

    // 七对：财神本就成对，视为满足
    if (hand.size == 14 && isSevenPairsCounts(counts, jokers) != null) return true

    // 有财神必须爆头：将牌中必须含财神
    if (mustBaoHead && jokers > 0) return pairUsesJoker(counts, jokers)

    // 选将牌（雀头），其余必须全部成面子
    for (p in 0 until JOKER) {
        if (counts[p] >= 2) {
            counts[p] -= 2
            val ok = canAllMelds(counts, jokers)
            counts[p] += 2
            if (ok) return true
        }
        if (counts[p] >= 1 && jokers >= 1) {
            counts[p] -= 1
            val ok = canAllMelds(counts, jokers - 1)
            counts[p] += 1
            if (ok) return true
        }
    }
    return jokers >= 2 && canAllMelds(counts, jokers - 2)
}

/** 七对判定（14 张，含财神）。返回 null 表示不是七对。 */
fun isSevenPairs(hand: IntArray): SevenPairs? {
    if (hand.size != 14) return null
    val counts = countsOf(hand)
    val jokers = counts[JOKER]
    counts[JOKER] = 0
    return isSevenPairsCounts(counts, jokers)
}

/** 13 张手牌的听口（可胡的牌），含财神白板。 */
fun winningTiles(hand: IntArray, mustBaoHead: Boolean = false): List<Int> {
    val counts = countsOf(hand)
    val result = ArrayList<Int>()
    for (c in 0 until KIND_COUNT) {
        if (counts[c] >= 4) continue // 该牌已用尽
        if (isWinningHand(hand + c, mustBaoHead)) result.add(c)
    }
    return result
}

/** 和牌计分：倍数与番型。 */
fun evaluateWin(
    hand: IntArray,
    melds: List<Meld> = emptyList(),
    opts: WinOptions = WinOptions(),
): WinResult {
    if (!isWinningHand(hand)) return WinResult.LOSE

    val counts = countsOf(hand)
    val jokers = counts[JOKER]
    counts[JOKER] = 0

    val seven = if (hand.size == 14 && melds.isEmpty()) isSevenPairsCounts(counts, jokers) else null

    val patterns = LinkedHashMap<String, Int>()
    if (opts.selfDraw) patterns["自摸"] = 1

    if (seven != null) {
        if (seven.usedJoker) patterns["七对子"] = 4 else patterns["清七对"] = 8
    }

    // 清一色 / 字一色：合上亮牌一起看
    val all = hand.toMutableList()
    for (m in melds) for (t in m.tiles) all.add(t)
    val suits = all.map { suitOf(it) }.toSet()
    if (suits.size == 1) {
        if (suits.first() == 3) patterns["字一色"] = 16 else patterns["清一色"] = 8
    }

    if (seven == null &&
        isAllTripletsHand(counts, jokers) &&
        melds.all { it.type == MeldType.PENG || it.type.isGang }
    ) {
        patterns["碰碰胡"] = 2
    }

    // 全倒：4 个面子全为明牌，手中只剩将牌
    if (melds.size == 4 && hand.size == 2) patterns["全倒"] = 2

    if (pairUsesJoker(counts, jokers)) patterns["爆头"] = 2

    when (opts.gangOpen) {
        MeldType.MING_GANG -> patterns["明杠开"] = 2
        MeldType.AN_GANG -> patterns["暗杠开"] = 4
        MeldType.BU_GANG -> patterns["补杠开"] = 2
        else -> {}
    }

    if (opts.jokerDiscarded && opts.selfDraw) patterns["财飘"] = 4
    if (opts.heavenly) patterns["天胡"] = 16
    if (opts.earthly) patterns["地胡"] = 16

    var mult = 1
    for (v in patterns.values) mult *= v
    if (opts.dealer) {
        mult *= when {
            opts.dealerStreak >= 2 -> 8
            opts.dealerStreak == 1 -> 4
            else -> 2
        }
    }
    mult = minOf(mult, 64)

    return WinResult(true, mult, patterns.keys.toList())
}
