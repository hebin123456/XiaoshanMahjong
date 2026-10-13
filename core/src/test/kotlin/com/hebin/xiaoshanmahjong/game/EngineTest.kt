package com.hebin.xiaoshanmahjong.game

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

private fun playRandom(rules: RuleSet, seed: Long): Game {
    val game = Game(rules, seed)
    var guard = 0
    while (!game.over) {
        if (guard++ > 8000) error("对局未能在合理步数内结束")
        when (game.phase) {
            Phase.DRAW -> game.drawFor(game.turn)
            Phase.DISCARD -> {
                val seat = game.turn
                if (game.checkSelfWin(seat) != null) {
                    game.finishSelfDraw(seat)
                    break
                }
                val gangs = game.selfGangOptions(seat)
                if (gangs.isNotEmpty() && game.rng.nextInt(2) == 0) {
                    val g = gangs[game.rng.nextInt(gangs.size)]
                    game.doSelfGang(seat, g.tile, g.type)
                } else {
                    val p = game.player(seat)
                    var r = game.discard(seat, p.hand[game.rng.nextInt(p.hand.size)])
                    var tries = 0
                    while (!r.ok && tries++ < 30) {
                        r = game.discard(seat, p.hand[game.rng.nextInt(p.hand.size)])
                    }
                    if (!r.ok) error("无法出牌: ${r.error}")
                }
            }
            Phase.CLAIM -> {
                val pd = game.pending!!
                for (cand in pd.candidates) {
                    val decision = if (game.rng.nextInt(10) < 6) {
                        when (cand.actions[game.rng.nextInt(cand.actions.size)]) {
                            "chi" -> ClaimDecision.Chi(cand.chiOptions[game.rng.nextInt(cand.chiOptions.size)])
                            "win" -> ClaimDecision.Win
                            "peng" -> ClaimDecision.Peng
                            "gang" -> ClaimDecision.Gang
                            else -> null
                        }
                    } else null
                    game.submitClaim(cand.seat, decision)
                }
                game.resolveClaims()
            }
            Phase.OVER -> break
        }
    }
    return game
}

class EngineTest {

    @Test
    fun dealCountsAreCorrect() {
        val g = Game(RuleSet(), seed = 7)
        assertEquals(14, g.player(g.dealer).hand.size)
        for (s in 0 until 4) if (s != g.dealer) assertEquals(13, g.player(s).hand.size)
        assertEquals(13, g.player(nextSeat(g.dealer)).hand.size)
        assertTrue(g.indicator in 0 until KIND_COUNT)
    }

    @Test
    fun randomGamesCompleteNoRush() {
        for (i in 0 until 10) {
            val g = playRandom(RuleSet(allowRush = false), 1000L + i)
            assertTrue(g.over)
            assertTrue(g.result != null)
        }
    }

    @Test
    fun randomGamesCompleteRush() {
        for (i in 0 until 10) {
            val g = playRandom(RuleSet(allowRush = true), 2000L + i)
            assertTrue(g.over)
            assertTrue(g.result != null)
        }
    }

    @Test
    fun tileConservation() {
        val g = playRandom(RuleSet(allowRush = true), 4321L)
        assertEquals(WALL_SIZE, g.totalTiles() + g.wall.size + 1) // +1 为财神翻牌
    }

    @Test
    fun scoresAreZeroSum() {
        val g = playRandom(RuleSet(allowRush = true), 88L)
        assertEquals(0, g.players.sumOf { it.score })
    }

    @Test
    fun chiThreeDisabledLimitsMeldsToTwo() {
        for (i in 0 until 15) {
            val g = playRandom(RuleSet(allowRush = true, allowChiThree = false), 5000L + i)
            for (p in g.players) assertTrue("摊数不应超过 2", p.melds.size <= 2)
        }
    }

    @Test
    fun chiThreeEnabledBindsContract() {
        val g = Game(RuleSet(allowRush = false, allowChiThree = true), seed = 1)
        val p0 = g.player(0)
        p0.hand.clear()
        p0.hand.addAll(
            listOf(
                tileIndex("z5"), tileIndex("m1"), tileIndex("m2"), tileIndex("m4"), tileIndex("m6"),
                tileIndex("s4"), tileIndex("s6"), tileIndex("s8"), tileIndex("p4"), tileIndex("p6"),
                tileIndex("p8"), tileIndex("z2"), tileIndex("z4"),
            ),
        )
        val p1 = g.player(1)
        p1.hand.clear()
        p1.melds.clear()
        p1.melds.add(Meld(MeldType.CHI, intArrayOf(tileIndex("m1"), tileIndex("m2"), tileIndex("m3")), 0))
        p1.melds.add(Meld(MeldType.PENG, intArrayOf(tileIndex("m5"), tileIndex("m5"), tileIndex("m5")), 2))
        p1.hand.addAll(
            listOf(
                tileIndex("z5"), tileIndex("z5"),
                tileIndex("m9"), tileIndex("m9"), tileIndex("m9"),
                tileIndex("p1"), tileIndex("p2"), tileIndex("p3"),
                tileIndex("s1"), tileIndex("s2"), tileIndex("s3"),
                tileIndex("m7"), tileIndex("m8"),
            ),
        )
        // 玩家 0 打出 z5，玩家 1 可碰，形成第 3 摊 → 触发承包
        val r = g.discard(0, tileIndex("z5"))
        assertTrue(r.ok)
        assertTrue(g.phase == Phase.CLAIM)
        g.submitClaim(1, ClaimDecision.Peng)
        g.resolveClaims()
        assertEquals(3, p1.melds.size)
        assertEquals(IntPair(1, 0), g.contractPair)
    }

    @Test
    fun chiThreeDisabledBlocksThirdMeld() {
        val g = Game(RuleSet(allowRush = false, allowChiThree = false), seed = 1)
        val p0 = g.player(0)
        p0.hand.clear()
        p0.hand.addAll(listOf(tileIndex("z5"), tileIndex("m1"), tileIndex("s4"), tileIndex("p6")))
        val p1 = g.player(1)
        p1.hand.clear()
        p1.melds.clear()
        p1.melds.add(Meld(MeldType.CHI, intArrayOf(tileIndex("m1"), tileIndex("m2"), tileIndex("m3")), 0))
        p1.melds.add(Meld(MeldType.PENG, intArrayOf(tileIndex("m5"), tileIndex("m5"), tileIndex("m5")), 2))
        p1.hand.addAll(listOf(tileIndex("z5"), tileIndex("z5"), tileIndex("p1"), tileIndex("p2")))
        // 清空其余家，避免随机手牌产生干扰候选
        g.player(2).hand.clear()
        g.player(3).hand.clear()
        val r = g.discard(0, tileIndex("z5"))
        assertTrue(r.ok)
        // 已满 2 摊，不允许再吃碰杠 → 无候选
        assertNull(g.pending)
    }
}
