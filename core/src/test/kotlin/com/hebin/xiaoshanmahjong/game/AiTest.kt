package com.hebin.xiaoshanmahjong.game

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotEquals
import org.junit.Assert.assertTrue
import org.junit.Test

private fun playAI(rules: RuleSet, seed: Long): Game {
    val game = Game(rules, seed)
    var guard = 0
    while (!game.over) {
        if (guard++ > 8000) error("AI 对局未能在合理步数内结束")
        when (game.phase) {
            Phase.DRAW -> game.drawFor(game.turn)
            Phase.DISCARD -> {
                val seat = game.turn
                if (game.checkSelfWin(seat) != null) {
                    game.finishSelfDraw(seat)
                    break
                }
                val gangs = game.selfGangOptions(seat)
                val g = chooseSelfGang(game, seat, gangs)
                if (g != null) {
                    game.doSelfGang(seat, g.tile, g.type)
                } else {
                    val r = game.discard(seat, chooseDiscard(game, seat))
                    if (!r.ok) error("AI 出牌被拒: ${r.error}")
                }
            }
            Phase.CLAIM -> {
                val pd = game.pending!!
                for (cand in pd.candidates) {
                    game.submitClaim(cand.seat, chooseClaim(game, cand.seat, cand))
                }
                game.resolveClaims()
            }
            Phase.OVER -> break
        }
    }
    return game
}

class AiTest {

    @Test
    fun aiNeverDiscardsJoker() {
        val g = Game(RuleSet(), seed = 3)
        val hand = g.player(0).hand
        hand.clear()
        hand.addAll(
            listOf(
                tileIndex("m1"), tileIndex("m2"), tileIndex("m3"), tileIndex("m4"), tileIndex("m5"),
                tileIndex("m6"), tileIndex("s1"), tileIndex("s2"), tileIndex("s3"), tileIndex("p7"),
                tileIndex("p8"), tileIndex("z7"), tileIndex("z1"), tileIndex("z4"),
            ),
        )
        g.turn = 0
        g.phase = Phase.DISCARD
        val t = chooseDiscard(g, 0)
        assertNotEquals(JOKER, t)
        assertTrue(hand.contains(t))
    }

    @Test
    fun aiChoosesWinWhenAvailable() {
        val g = Game(RuleSet(allowRush = true), seed = 5)
        val p1 = g.player(1)
        p1.hand.clear()
        p1.hand.addAll(
            listOf(
                tileIndex("m1"), tileIndex("m2"), tileIndex("m3"), tileIndex("m4"), tileIndex("m5"),
                tileIndex("m6"), tileIndex("s1"), tileIndex("s2"), tileIndex("s3"), tileIndex("p7"),
                tileIndex("p8"), tileIndex("z1"), tileIndex("z1"),
            ),
        )
        g.pending = Pending(
            tileIndex("p9"),
            0,
            listOf(ClaimCandidate(1, listOf("win", "peng"), emptyList())),
        )
        val d = chooseClaim(g, 1, g.pending!!.candidates[0])
        assertEquals(ClaimDecision.Win, d)
    }

    @Test
    fun structureScoreRanksCompleteSetsHigher() {
        val good = structureScore(listOf(tileIndex("m1"), tileIndex("m2"), tileIndex("m3"), tileIndex("m4"), tileIndex("m5"), tileIndex("m6")))
        val bad = structureScore(listOf(tileIndex("m1"), tileIndex("m5"), tileIndex("z1"), tileIndex("z4"), tileIndex("p3"), tileIndex("s9")))
        assertTrue(good > bad)
    }

    @Test
    fun aiGamesCompleteAllLevels() {
        for (level in AiLevel.values()) {
            for (i in 0 until 8) {
                val rules = RuleSet(allowRush = false, aiLevel = level)
                assertTrue(playAI(rules, 3000L + i).over)
            }
            for (i in 0 until 8) {
                val rules = RuleSet(allowRush = true, allowChiThree = true, aiLevel = level)
                assertTrue(playAI(rules, 4000L + i).over)
            }
        }
    }
}
