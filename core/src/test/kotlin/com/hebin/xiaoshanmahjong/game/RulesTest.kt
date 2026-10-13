package com.hebin.xiaoshanmahjong.game

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test
import kotlin.random.Random

private fun h(vararg names: String): IntArray = names.map { tileIndex(it) }.toIntArray()

class RulesTest {

    @Test
    fun wallHas136TilesFourEach() {
        val wall = makeWall(Random(1))
        assertEquals(136, wall.size)
        val counts = countsOf(wall)
        assertEquals(KIND_COUNT, counts.size)
        for (c in counts) assertEquals(4, c)
    }

    @Test
    fun standardHandWins() {
        assertTrue(isWinningHand(h("m1", "m1", "m1", "m2", "m3", "m4", "s1", "s2", "s3", "p5", "p6", "p7", "z1", "z1")))
    }

    @Test
    fun incompleteHandDoesNotWin() {
        assertFalse(isWinningHand(h("m1", "m1", "m1", "m2", "m3", "m4", "s1", "s2", "s3", "p5", "p6", "p7", "z1", "z2")))
    }

    @Test
    fun jokerCanCompleteSequence() {
        assertTrue(isWinningHand(h("m1", "m2", "m3", "m4", "m5", "m6", "s1", "s2", "s3", "p7", "p8", "z7", "z1", "z1")))
    }

    @Test
    fun scatteredTilesDoNotWin() {
        assertFalse(isWinningHand(h("m1", "m2", "m4", "m5", "m7", "m8", "s1", "s2", "s4", "s5", "p1", "p3", "p5", "z2")))
    }

    @Test
    fun naturalSevenPairs() {
        val r = isSevenPairs(h("m1", "m1", "m2", "m2", "m3", "m3", "s1", "s1", "s2", "s2", "p1", "p1", "z1", "z1"))
        assertNotNull(r)
        assertFalse(r!!.usedJoker)
    }

    @Test
    fun sevenPairsWithJoker() {
        val r = isSevenPairs(h("m1", "m1", "m2", "m2", "m3", "m3", "s1", "s1", "s2", "s2", "p1", "p1", "z1", "z7"))
        assertNotNull(r)
        assertTrue(r!!.usedJoker)
    }

    @Test
    fun waitingTilesAreCorrect() {
        val t = winningTiles(h("m1", "m2", "m3", "m4", "m5", "m6", "s1", "s2", "s3", "p7", "p8", "z1", "z1"))
        assertEquals(listOf(tileIndex("p6"), tileIndex("p9"), tileIndex("z7")), t.sorted())
    }

    @Test
    fun plainSelfDrawIsOne() {
        val r = evaluateWin(h("m1", "m1", "m1", "m2", "m3", "m4", "s1", "s2", "s3", "p5", "p6", "p7", "z1", "z1"), emptyList(), WinOptions(selfDraw = true))
        assertTrue(r.win)
        assertEquals(1, r.multiplier)
    }

    @Test
    fun allTripletsDoubles() {
        val r = evaluateWin(h("m1", "m1", "m1", "m2", "m2", "m2", "s1", "s1", "s1", "p5", "p5", "p5", "z1", "z1"), emptyList(), WinOptions(selfDraw = true))
        assertEquals(2, r.multiplier)
        assertTrue(r.patterns.contains("碰碰胡"))
    }

    @Test
    fun pureSuitScoresEight() {
        val r = evaluateWin(h("m1", "m1", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9", "m9", "m9", "m9"), emptyList(), WinOptions(selfDraw = true))
        assertTrue(r.patterns.contains("清一色"))
        assertTrue(r.multiplier >= 8)
    }

    @Test
    fun dealerMultiplierIncreases() {
        val hand = h("m1", "m1", "m1", "m2", "m3", "m4", "s1", "s2", "s3", "p5", "p6", "p7", "z1", "z1")
        val r1 = evaluateWin(hand, emptyList(), WinOptions(selfDraw = true, dealer = true, dealerStreak = 0))
        val r2 = evaluateWin(hand, emptyList(), WinOptions(selfDraw = true, dealer = true, dealerStreak = 1))
        assertTrue(r2.multiplier >= r1.multiplier)
    }

    @Test
    fun mustBaoHeadRejectsJokerInSequence() {
        val hand = h("m1", "m2", "m3", "m4", "m5", "m6", "s1", "s2", "s3", "p7", "p8", "z7", "z1", "z1")
        assertTrue(isWinningHand(hand, mustBaoHead = false))
        assertFalse(isWinningHand(hand, mustBaoHead = true))
    }

    @Test
    fun mustBaoHeadAcceptsJokerAsPair() {
        val hand = h("m1", "m2", "m3", "m4", "m5", "m6", "s1", "s2", "s3", "p1", "p2", "p3", "z1", "z7")
        assertTrue(isWinningHand(hand, mustBaoHead = true))
    }

    @Test
    fun noJokerUnaffectedByMustBaoHead() {
        val hand = h("m1", "m1", "m1", "m2", "m3", "m4", "s1", "s2", "s3", "p5", "p6", "p7", "z1", "z1")
        assertTrue(isWinningHand(hand, mustBaoHead = true))
        assertNull(isSevenPairs(hand))
    }
}
