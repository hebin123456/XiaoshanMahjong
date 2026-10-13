package com.hebin.xiaoshanmahjong.game

/**
 * 结构评分：把牌拆成尽可能多的面子（刻子 / 顺子）与对子，分数越高越接近和牌。
 * 财神按高价值计算。
 */
fun structureScore(tiles: List<Int>): Int {
    val counts = countsOf(tiles)
    val jokers = counts[JOKER]
    counts[JOKER] = 0
    var score = 0

    // 刻子
    for (i in 0 until KIND_COUNT) {
        while (counts[i] >= 3) {
            counts[i] -= 3
            score += 3
        }
    }
    // 顺子
    for (i in 0 until 27) {
        while (rankOf(i) <= 7 && counts[i] > 0 && counts[i + 1] > 0 && counts[i + 2] > 0) {
            counts[i]--
            counts[i + 1]--
            counts[i + 2]--
            score += 3
        }
    }
    // 对子
    for (i in 0 until KIND_COUNT) if (counts[i] >= 2) score += 1
    // 财神本身很有价值
    score += jokers * 3
    return score
}

/** 选择要打出的牌（绝不主动打财神）。 */
fun chooseDiscard(game: Game, seat: Int, level: AiLevel = game.rules.aiLevel): Int {
    val hand = game.player(seat).hand
    val candidates = hand.toSet().filter { it != JOKER }
    if (candidates.isEmpty()) return hand[0]

    if (level == AiLevel.EASY) {
        return candidates[game.rng.nextInt(candidates.size)]
    }

    var best = candidates[0]
    var bestScore = Int.MIN_VALUE
    val counts = countsOf(hand)
    for (t in candidates) {
        val rest = hand.toMutableList().also { it.remove(t) }
        var s = structureScore(rest) * 100
        // 听牌倾向：剩余 13 张时看听口数量
        if (level == AiLevel.HARD && rest.size % 3 == 1) {
            s += winningTiles(rest.toIntArray(), game.rules.mustBaoHead).size * 10
        }
        // 优先打孤张（剩余张数少者更安全）
        s += (4 - counts[t])
        if (s > bestScore) {
            bestScore = s
            best = t
        }
    }
    return best
}

/** 选择是否自杠。 */
fun chooseSelfGang(game: Game, seat: Int, options: List<GangOption>, level: AiLevel = game.rules.aiLevel): GangOption? {
    if (options.isEmpty()) return null
    if (level == AiLevel.EASY && game.rng.nextInt(2) == 0) return null
    return options[0]
}

/** 选择对弃牌的响应（胡 > 杠 > 碰 > 吃）。 */
fun chooseClaim(game: Game, seat: Int, cand: ClaimCandidate, level: AiLevel = game.rules.aiLevel): ClaimDecision? {
    if ("win" in cand.actions) return ClaimDecision.Win

    if (level == AiLevel.EASY) {
        val opts = cand.actions.filter { it != "win" }
        if (opts.isEmpty()) return null
        return when (opts[game.rng.nextInt(opts.size)]) {
            "gang" -> ClaimDecision.Gang
            "peng" -> ClaimDecision.Peng
            "chi" -> ClaimDecision.Chi(cand.chiOptions[game.rng.nextInt(cand.chiOptions.size)])
            else -> null
        }
    }

    val tile = game.pending?.tile ?: return null
    val hand = game.player(seat).hand
    val before = structureScore(hand)

    if ("gang" in cand.actions) return ClaimDecision.Gang

    if ("peng" in cand.actions) {
        val rest = hand.toMutableList().also { repeat(2) { _ -> it.remove(tile) } }
        // 碰出刻子 +3；只要不明显破坏结构就碰
        if (structureScore(rest) + 3 >= before - 1) return ClaimDecision.Peng
    }

    if ("chi" in cand.actions) {
        var bestChi: IntArray? = null
        var bestScore = before
        for (two in cand.chiOptions) {
            val rest = hand.toMutableList().also { for (x in two) it.remove(x) }
            val s = structureScore(rest) + 3
            if (s > bestScore) {
                bestScore = s
                bestChi = two
            }
        }
        if (bestChi != null) return ClaimDecision.Chi(bestChi)
    }

    return null
}
