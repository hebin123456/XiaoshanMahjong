package com.hebin.xiaoshanmahjong.ui

import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.size
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.rotate
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.Dp
import com.hebin.xiaoshanmahjong.R
import com.hebin.xiaoshanmahjong.game.JOKER
import com.hebin.xiaoshanmahjong.game.KIND_COUNT
import com.hebin.xiaoshanmahjong.game.tileName

/** 牌索引（0..33）→ 贴图资源。 */
val TILE_RES: IntArray = intArrayOf(
    R.drawable.tile_m1, R.drawable.tile_m2, R.drawable.tile_m3, R.drawable.tile_m4, R.drawable.tile_m5,
    R.drawable.tile_m6, R.drawable.tile_m7, R.drawable.tile_m8, R.drawable.tile_m9,
    R.drawable.tile_s1, R.drawable.tile_s2, R.drawable.tile_s3, R.drawable.tile_s4, R.drawable.tile_s5,
    R.drawable.tile_s6, R.drawable.tile_s7, R.drawable.tile_s8, R.drawable.tile_s9,
    R.drawable.tile_p1, R.drawable.tile_p2, R.drawable.tile_p3, R.drawable.tile_p4, R.drawable.tile_p5,
    R.drawable.tile_p6, R.drawable.tile_p7, R.drawable.tile_p8, R.drawable.tile_p9,
    R.drawable.tile_z1, R.drawable.tile_z2, R.drawable.tile_z3, R.drawable.tile_z4, R.drawable.tile_z5,
    R.drawable.tile_z6, R.drawable.tile_z7,
).also { check(it.size == KIND_COUNT) }

/** 麻将牌正面。aspect 为 3:4（宽:高）。 */
@Composable
fun TileFace(index: Int, width: Dp, height: Dp, modifier: Modifier = Modifier) {
    Image(
        painter = painterResource(TILE_RES[index]),
        contentDescription = tileName(index),
        contentScale = ContentScale.Fit,
        modifier = modifier.size(width, height),
    )
}

/** 牌背（对家手牌）。 */
@Composable
fun TileBack(width: Dp, height: Dp, modifier: Modifier = Modifier) {
    Image(
        painter = painterResource(R.drawable.tile_back),
        contentDescription = "牌背",
        contentScale = ContentScale.Fit,
        modifier = modifier.size(width, height),
    )
}

/** 旋转 90° 的牌背（左右两家手牌）。 */
@Composable
fun TileBackRotated(tileW: Dp, tileH: Dp, modifier: Modifier = Modifier) {
    Box(modifier.size(tileH, tileW), contentAlignment = Alignment.Center) {
        TileBack(tileW, tileH, Modifier.rotate(90f))
    }
}

/** 财神（白板）提示用小牌。 */
@Composable
fun JokerBadge(width: Dp, height: Dp) {
    TileFace(JOKER, width, height)
}
