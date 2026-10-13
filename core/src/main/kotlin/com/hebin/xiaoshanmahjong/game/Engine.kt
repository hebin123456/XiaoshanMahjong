package com.hebin.xiaoshanmahjong.game

import kotlin.random.Random

/** 顺时针下一家。 */
fun nextSeat(seat: Int): Int = (seat + 1) % 4

enum class Phase { DRAW, DISCARD, CLAIM, OVER }

class PlayerState(val seat: Int) {
    val hand = ArrayList<Int>()
    val melds = ArrayList<Meld>()
    val discards = ArrayList<Int>()
    var score = 0
    var drawCount = 0
}

data class ClaimCandidate(val seat: Int, val actions: List<String>, val chiOptions: List<IntArray>)

data class Pending(val tile: Int, val from: Int, val candidates: List<ClaimCandidate>)

data class IntPair(val first: Int, val second: Int)

data class GangOption(val tile: Int, val type: MeldType)

data class MoveResult(val error: String? = null) {
    val ok: Boolean get() = error == null
}

data class GameResult(
    val winner: Int,
    val discarder: Int,
    val selfDraw: Boolean,
    val win: WinResult,
    val drawnGame: Boolean,
    val contractPair: IntPair? = null,
)

/** 玩家对弃牌的响应。 */
sealed interface ClaimDecision {
    data class Chi(val tiles: IntArray) : ClaimDecision
    data object Win : ClaimDecision
    data object Peng : ClaimDecision
    data object Gang : ClaimDecision
}

/**
 * 一局萧山麻将的状态机：发牌 → 摸牌 → 出牌 → 吃碰杠胡 → 结算。
 * 纯逻辑、无 Android 依赖，便于单元测试。
 */
class Game(
    val rules: RuleSet = RuleSet(),
    seed: Long = System.nanoTime(),
    val dealerStreak: Int = 0,
) {
    val rng = Random(seed)
    val wall = ArrayList<Int>()
    val players = Array(4) { PlayerState(it) }

    var dealer: Int = rng.nextInt(4)
    var turn: Int = dealer
    var phase: Phase = Phase.DISCARD

    /** 财神翻牌（仅展示用）。 */
    var indicator: Int = -1

    var pending: Pending? = null
    var lastDiscardTile: Int = -1
    var lastDiscardFrom: Int = -1
    var over: Boolean = false
    var result: GameResult? = null

    /** 本局是否已有财神（白板）被打出。 */
    var jokerDiscarded: Boolean = false

    /** 本回合是否杠后补牌（用于杠开番型）。 */
    var gangOpenThisTurn: MeldType? = null

    /** 承包绑定：(吃太多的人 A, 供牌方 B)。 */
    var contractPair: IntPair? = null

    private var discardsMade = 0
    private val decisions = HashMap<Int, ClaimDecision?>()

    init {
        val w = makeWall(rng)
        for (t in w) wall.add(t)
        repeat(13) { for (s in 0 until 4) player(s).hand.add(drawTile()) }
        player(dealer).hand.add(drawTile())
        indicator = drawTile()
        for (p in players) p.hand.sort()
        turn = dealer
        phase = Phase.DISCARD
    }

    fun player(seat: Int): PlayerState = players[seat]

    private fun drawTile(): Int = wall.removeAt(wall.lastIndex)

    /** 摸牌；牌墙摸完则流局。 */
    fun drawFor(seat: Int) {
        if (wall.isEmpty()) {
            finishDrawGame()
            return
        }
        val p = player(seat)
        val t = drawTile()
        p.hand.add(t)
        p.hand.sort()
        p.drawCount++
        turn = seat
        phase = Phase.DISCARD
    }

    /** 当前手牌是否可自摸胡；可胡返回计分明细，否则 null。 */
    fun checkSelfWin(seat: Int): WinResult? {
        val p = player(seat)
        val hand = p.hand.toIntArray()
        if (!isWinningHand(hand, rules.mustBaoHead)) return null
        val opts = WinOptions(
            selfDraw = true,
            dealer = seat == dealer,
            dealerStreak = if (seat == dealer) dealerStreak else 0,
            gangOpen = gangOpenThisTurn,
            jokerDiscarded = jokerDiscarded,
            heavenly = seat == dealer && discardsMade == 0,
            earthly = seat != dealer && p.drawCount == 1 && p.melds.isEmpty(),
        )
        return evaluateWin(hand, p.melds, opts)
    }

    /** 可自摸胡时结算。 */
    fun finishSelfDraw(seat: Int) {
        val r = checkSelfWin(seat) ?: return
        settle(seat, -1, true, r)
    }

    /** 可杠的选项：暗杠（手中 4 张）与补杠（已碰 + 手中 1 张）。 */
    fun selfGangOptions(seat: Int): List<GangOption> {
        val p = player(seat)
        if (!rules.allowChiThree && p.melds.size >= 2) return emptyList()
        val c = countsOf(p.hand)
        val res = ArrayList<GangOption>()
        for (i in 0 until KIND_COUNT) if (c[i] == 4) res.add(GangOption(i, MeldType.AN_GANG))
        for (m in p.melds) if (m.type == MeldType.PENG && c[m.tiles[0]] >= 1) {
            res.add(GangOption(m.tiles[0], MeldType.BU_GANG))
        }
        return res
    }

    /** 执行暗杠 / 补杠并补牌。 */
    fun doSelfGang(seat: Int, tile: Int, type: MeldType) {
        val p = player(seat)
        if (type == MeldType.AN_GANG) {
            repeat(4) { p.hand.remove(tile) }
            p.melds.add(Meld(MeldType.AN_GANG, intArrayOf(tile, tile, tile, tile)))
        } else {
            p.hand.remove(tile)
            val idx = p.melds.indexOfFirst { it.type == MeldType.PENG && it.tiles[0] == tile }
            if (idx >= 0) {
                val old = p.melds[idx]
                p.melds[idx] = Meld(MeldType.BU_GANG, intArrayOf(tile, tile, tile, tile), old.from)
            }
        }
        gangOpenThisTurn = type
        drawFor(seat)
    }

    /** 打出一张牌并计算其他家可吃碰杠胡的候选。 */
    fun discard(seat: Int, tile: Int): MoveResult {
        val p = player(seat)
        val idx = p.hand.indexOf(tile)
        if (idx < 0) return MoveResult("手牌中没有这张牌")
        p.hand.removeAt(idx)
        p.discards.add(tile)
        lastDiscardTile = tile
        lastDiscardFrom = seat
        if (tile == JOKER) jokerDiscarded = true
        discardsMade++
        gangOpenThisTurn = null
        computeCandidates(tile, seat)
        return MoveResult()
    }

    private fun computeCandidates(tile: Int, from: Int) {
        val cands = ArrayList<ClaimCandidate>()
        for (s in 0 until 4) {
            if (s == from) continue
            val p = player(s)
            val acts = ArrayList<String>()
            val chis = ArrayList<IntArray>()
            if (rules.allowRush && isWinningHand((p.hand + tile).toIntArray(), rules.mustBaoHead)) {
                acts.add("win")
            }
            if (tile != JOKER) {
                val meldLimited = !rules.allowChiThree && p.melds.size >= 2
                if (!meldLimited) {
                    if (p.hand.count { it == tile } >= 3) acts.add("gang")
                    if (p.hand.count { it == tile } >= 2) acts.add("peng")
                    if (s == nextSeat(from)) {
                        val opts = chiOptions(p.hand, tile)
                        if (opts.isNotEmpty()) {
                            acts.add("chi")
                            chis.addAll(opts)
                        }
                    }
                }
            }
            if (acts.isNotEmpty()) cands.add(ClaimCandidate(s, acts, chis))
        }
        if (cands.isEmpty()) {
            pending = null
            turn = nextSeat(from)
            phase = Phase.DRAW
        } else {
            pending = Pending(tile, from, cands)
            phase = Phase.CLAIM
        }
    }

    private fun chiOptions(hand: List<Int>, tile: Int): List<IntArray> {
        if (isHonor(tile)) return emptyList()
        val c = countsOf(hand)
        val rank = rankOf(tile)
        val res = ArrayList<IntArray>()
        fun has(x: Int) = x in 0 until 27 && c[x] > 0
        if (rank >= 3) {
            val a = tile - 2
            val b = tile - 1
            if (has(a) && has(b)) res.add(intArrayOf(a, b))
        }
        if (rank in 2..8) {
            val a = tile - 1
            val b = tile + 1
            if (has(a) && has(b)) res.add(intArrayOf(a, b))
        }
        if (rank <= 7) {
            val a = tile + 1
            val b = tile + 2
            if (has(a) && has(b)) res.add(intArrayOf(a, b))
        }
        return res
    }

    fun submitClaim(seat: Int, decision: ClaimDecision?) {
        decisions[seat] = decision
    }

    /** 按 胡 > 杠 > 碰 > 吃 的优先级结算各家响应。 */
    fun resolveClaims() {
        val pd = pending
        if (pd == null) {
            decisions.clear()
            return
        }
        val ms = pd.candidates.mapNotNull { c -> decisions[c.seat]?.let { c.seat to it } }
        decisions.clear()

        val win = ms.firstOrNull { it.second is ClaimDecision.Win }
        if (win != null) {
            claimWin(win.first, pd)
            return
        }
        val gang = ms.firstOrNull { it.second is ClaimDecision.Gang }
        if (gang != null) {
            applyMingGang(gang.first, pd)
            return
        }
        val peng = ms.firstOrNull { it.second is ClaimDecision.Peng }
        if (peng != null) {
            applyPeng(peng.first, pd)
            return
        }
        val chi = ms.firstOrNull { it.second is ClaimDecision.Chi }
        if (chi != null) {
            applyChi(chi.first, pd, (chi.second as ClaimDecision.Chi).tiles)
            return
        }
        pending = null
        turn = nextSeat(pd.from)
        phase = Phase.DRAW
    }

    private fun unDiscard(from: Int) {
        val d = player(from).discards
        if (d.isNotEmpty()) d.removeAt(d.size - 1)
    }

    private fun applyPeng(seat: Int, pd: Pending) {
        val p = player(seat)
        repeat(2) { p.hand.remove(pd.tile) }
        p.melds.add(Meld(MeldType.PENG, intArrayOf(pd.tile, pd.tile, pd.tile), pd.from))
        unDiscard(pd.from)
        pending = null
        maybeBindContract(seat, pd.from)
        turn = seat
        phase = Phase.DISCARD
    }

    private fun applyChi(seat: Int, pd: Pending, two: IntArray) {
        val p = player(seat)
        for (t in two) p.hand.remove(t)
        val tiles = (intArrayOf(pd.tile) + two).sortedArray()
        p.melds.add(Meld(MeldType.CHI, tiles, pd.from))
        unDiscard(pd.from)
        pending = null
        maybeBindContract(seat, pd.from)
        turn = seat
        phase = Phase.DISCARD
    }

    private fun applyMingGang(seat: Int, pd: Pending) {
        val p = player(seat)
        repeat(3) { p.hand.remove(pd.tile) }
        p.melds.add(Meld(MeldType.MING_GANG, intArrayOf(pd.tile, pd.tile, pd.tile, pd.tile), pd.from))
        unDiscard(pd.from)
        pending = null
        maybeBindContract(seat, pd.from)
        gangOpenThisTurn = MeldType.MING_GANG
        turn = seat
        drawFor(seat)
    }

    private fun claimWin(seat: Int, pd: Pending) {
        val p = player(seat)
        val hand = (p.hand + pd.tile).toIntArray()
        val opts = WinOptions(
            selfDraw = false,
            dealer = seat == dealer,
            dealerStreak = if (seat == dealer) dealerStreak else 0,
            jokerDiscarded = jokerDiscarded,
        )
        val r = evaluateWin(hand, p.melds, opts)
        settle(seat, pd.from, false, r)
    }

    private fun maybeBindContract(seat: Int, from: Int) {
        if (!rules.allowChiThree) return
        if (contractPair != null) return
        if (from >= 0 && player(seat).melds.size >= 3) contractPair = IntPair(seat, from)
    }

    private fun settle(winner: Int, discarder: Int, selfDraw: Boolean, r: WinResult) {
        val mult = r.multiplier
        val cp = contractPair

        if (cp != null && (winner == cp.first || winner == cp.second)) {
            // 绑定双方之间出胜者：另一方独付全部，并加倍
            val loser = if (winner == cp.first) cp.second else cp.first
            val amount = mult * 3 * 2
            player(loser).score -= amount
            player(winner).score += amount
        } else if (cp != null) {
            // 其余人赢：供牌方的钱由吃太多的人出
            if (selfDraw) {
                for (s in 0 until 4) {
                    if (s == winner) continue
                    if (s == cp.second) player(cp.first).score -= mult else player(s).score -= mult
                }
                player(winner).score += mult * 3
            } else {
                val payer = if (discarder == cp.second) cp.first else discarder
                player(payer).score -= mult * 3
                player(winner).score += mult * 3
            }
        } else if (selfDraw) {
            for (s in 0 until 4) {
                if (s == winner) continue
                player(s).score -= mult
                player(winner).score += mult
            }
        } else {
            player(discarder).score -= mult * 3
            player(winner).score += mult * 3
        }

        result = GameResult(winner, discarder, selfDraw, r, false, cp)
        over = true
        phase = Phase.OVER
        dealer = winner // 抢庄制：谁胡谁做庄
    }

    private fun finishDrawGame() {
        result = GameResult(-1, -1, false, WinResult.LOSE, true, contractPair)
        over = true
        phase = Phase.OVER
    }

    /** 手牌 + 亮牌 + 弃牌的总张数（用于守恒自检）。 */
    fun totalTiles(): Int {
        var n = 0
        for (p in players) {
            n += p.hand.size
            for (m in p.melds) n += m.tiles.size
            n += p.discards.size
        }
        return n
    }
}
