package com.hebin.xiaoshanmahjong.game

import kotlin.random.Random

/** 牌型种类数：m1..m9 (0..8)、s1..s9 (9..17)、p1..p9 (18..26)、z1..z7 (27..33)。 */
const val KIND_COUNT = 34

/** z7（白板）为万能财神 / 百搭牌。 */
const val JOKER = 33

/** 一副牌 136 张（每种 4 张，无花牌）。 */
const val WALL_SIZE = 136

/** 索引 → 牌名，如 0 -> "m1"、33 -> "z7"。 */
val TILE_NAMES: List<String> = buildList {
    for (suit in charArrayOf('m', 's', 'p')) for (n in 1..9) add("$suit$n")
    for (n in 1..7) add("z$n")
}

/** 牌名 → 索引，如 "m1" -> 0、"z7" -> 33。 */
fun tileIndex(name: String): Int {
    val suit = name[0]
    val n = name.substring(1).toInt()
    return when (suit) {
        'm' -> n - 1
        's' -> 9 + n - 1
        'p' -> 18 + n - 1
        'z' -> 27 + n - 1
        else -> throw IllegalArgumentException("未知牌名: $name")
    }
}

fun tileName(index: Int): String = TILE_NAMES[index]

/** 0=万 1=索 2=筒 3=字。 */
fun suitOf(index: Int): Int = when {
    index < 9 -> 0
    index < 18 -> 1
    index < 27 -> 2
    else -> 3
}

fun isHonor(index: Int): Boolean = index >= 27

/** 花色内序号：m1 -> 1 ... z1 -> 1 ... z7 -> 7。 */
fun rankOf(index: Int): Int = if (index < 27) index % 9 + 1 else index - 26

/** 洗好的牌墙（136 张），使用给定随机源，便于复现。 */
fun makeWall(rng: Random): IntArray {
    val wall = IntArray(WALL_SIZE) { it / 4 }
    for (i in wall.indices.reversed()) {
        val j = rng.nextInt(i + 1)
        val t = wall[i]
        wall[i] = wall[j]
        wall[j] = t
    }
    return wall
}

fun countsOf(tiles: IntArray): IntArray {
    val c = IntArray(KIND_COUNT)
    for (t in tiles) c[t]++
    return c
}

fun countsOf(tiles: Collection<Int>): IntArray {
    val c = IntArray(KIND_COUNT)
    for (t in tiles) c[t]++
    return c
}
