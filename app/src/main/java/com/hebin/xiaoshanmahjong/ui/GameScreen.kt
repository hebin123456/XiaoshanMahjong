package com.hebin.xiaoshanmahjong.ui

import androidx.compose.animation.core.animateOffsetAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.rotate
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.rotate
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.Density
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.hebin.xiaoshanmahjong.game.*

fun seatName(seat: Int): String = when (seat) {
    0 -> "你"
    1 -> "下家"
    2 -> "对家"
    else -> "上家"
}

@Composable
fun GameScreen(rules: RuleSet, key: Int, onExit: () -> Unit, onRestart: () -> Unit) {
    val controller = remember(key) { GameController(rules) }
    LaunchedEffect(controller) { controller.run() }

    val rev = controller.rev
    val game = controller.game
    val prompt = controller.prompt

    BoxWithConstraints(
        modifier = Modifier
            .fillMaxSize()
            .background(TableColors.FeltDark),
    ) {
        val height = maxHeight
        val handTileH = height * 0.20f
        val handTileW = handTileH * 0.75f
        val midTileH = height * 0.10f
        val midTileW = midTileH * 0.75f
        val sideTile = height * 0.075f

        // 顶部返回 / 规则摘要
        Row(
            modifier = Modifier
                .align(Alignment.TopStart)
                .padding(8.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            TextButton(onClick = onExit) { Text("‹ 返回", color = TableColors.Accent) }
            Text(ruleSummary(rules), color = TableColors.Accent.copy(alpha = 0.6f), fontSize = 12.sp)
        }

        // 中央信息
        CenterInfo(
            game = game,
            tileW = midTileW,
            tileH = midTileH,
            modifier = Modifier.align(Alignment.Center),
        )

        // 对家（顶）
        OpponentColumn(
            game = game,
            seat = 2,
            tileW = sideTile,
            tileH = sideTile * 1.33f,
            modifier = Modifier.align(Alignment.TopCenter),
        )

        // 上家（左）
        OpponentSide(
            game = game,
            seat = 3,
            tileW = sideTile,
            tileH = sideTile * 1.33f,
            modifier = Modifier.align(Alignment.CenterStart),
        )

        // 下家（右）
        OpponentSide(
            game = game,
            seat = 1,
            tileW = sideTile,
            tileH = sideTile * 1.33f,
            modifier = Modifier.align(Alignment.CenterEnd),
        )

        // 我方弃牌（手上方）
        DiscardRow(
            tiles = game.player(HUMAN).discards,
            tileW = midTileW,
            tileH = midTileH,
            modifier = Modifier
                .align(Alignment.BottomCenter)
                .padding(bottom = handTileH + 44.dp),
        )

        // 我方手牌 + 亮牌
        HumanArea(
            controller = controller,
            game = game,
            prompt = prompt,
            tileW = handTileW,
            tileH = handTileH,
            modifier = Modifier.align(Alignment.BottomCenter),
        )

        // 手部动画：随当前行动方移动
        val density = LocalDensity.current
        val target = with(density) { handAnchor(game, maxWidth, height) }
        val handPos by animateOffsetAsState(targetValue = target, animationSpec = tween(360), label = "hand")
        Canvas(Modifier.fillMaxSize()) {
            drawHand(handPos, rotateForSeat(game.turn), 40f)
        }

        // 操作按钮
        if (prompt != Prompt.NONE) {
            ActionBar(
                controller = controller,
                game = game,
                prompt = prompt,
                modifier = Modifier.align(Alignment.Center),
            )
        }

        if (game.over) {
            ResultOverlay(game = game, onRestart = onRestart, onExit = onExit)
        }
    }
}

private fun ruleSummary(rules: RuleSet): String {
    val rush = if (rules.allowRush) "允许捉冲" else "自摸专属"
    val bao = if (rules.mustBaoHead) "财神须爆头" else "财神随意"
    val chi = if (rules.allowChiThree) "允许吃三摊" else "最多两摊"
    return "AI:${aiLevelName(rules.aiLevel)} · $rush · $bao · $chi"
}

/** 当前行动方的锚点（手应该在的位置），返回像素坐标。 */
private fun Density.handAnchor(game: Game, w: Dp, h: Dp): Offset {
    val wpx = w.toPx()
    val hpx = h.toPx()
    val cx = wpx / 2f
    val cy = hpx / 2f
    return when {
        game.phase == Phase.CLAIM -> Offset(cx, cy + hpx * 0.05f)
        game.turn == 0 -> Offset(cx, hpx * 0.82f)
        game.turn == 1 -> Offset(wpx * 0.80f, cy + hpx * 0.12f)
        game.turn == 2 -> Offset(cx, hpx * 0.22f)
        else -> Offset(wpx * 0.20f, cy + hpx * 0.12f)
    }
}

private fun rotateForSeat(seat: Int): Float = when (seat) {
    0 -> 0f
    1 -> -90f
    2 -> 180f
    else -> 90f
}

private fun DrawScope.drawHand(center: Offset, rotationDeg: Float, sizePx: Float) {
    rotate(degrees = rotationDeg, pivot = center) {
        val s = sizePx
        val palmW = s * 0.62f
        val palmH = s * 0.55f
        val skin = Color(0xFFF2D3B3)
        val skinEdge = Color(0xFFD9A97F)
        // 掌心
        drawRoundRect(
            color = skin,
            topLeft = Offset(center.x - palmW / 2f, center.y - palmH / 2f + s * 0.18f),
            size = Size(palmW, palmH),
            cornerRadius = CornerRadius(palmW * 0.3f),
        )
        // 四根手指
        val fingerW = palmW * 0.2f
        val fingerH = s * 0.42f
        for (i in 0 until 4) {
            val x = center.x - palmW / 2f + palmW * 0.08f + i * (palmW * 0.24f)
            drawRoundRect(
                color = skin,
                topLeft = Offset(x, center.y - fingerH / 2f - s * 0.06f),
                size = Size(fingerW, fingerH),
                cornerRadius = CornerRadius(fingerW / 2f),
            )
        }
        // 拇指
        drawRoundRect(
            color = skin,
            topLeft = Offset(center.x + palmW * 0.18f, center.y + s * 0.06f),
            size = Size(fingerW, s * 0.26f),
            cornerRadius = CornerRadius(fingerW / 2f),
        )
        // 描边
        drawRoundRect(
            color = skinEdge,
            topLeft = Offset(center.x - palmW / 2f, center.y - palmH / 2f + s * 0.18f),
            size = Size(palmW, palmH),
            cornerRadius = CornerRadius(palmW * 0.3f),
            style = androidx.compose.ui.graphics.drawscope.Stroke(width = 2f),
        )
    }
}

@Composable
private fun CenterInfo(game: Game, tileW: Dp, tileH: Dp, modifier: Modifier = Modifier) {
    Surface(
        modifier = modifier,
        shape = MaterialTheme.shapes.medium,
        color = TableColors.Felt.copy(alpha = 0.75f),
    ) {
        Column(
            modifier = Modifier.padding(horizontal = 18.dp, vertical = 10.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text("财神", color = TableColors.Gold, fontSize = 12.sp)
                Spacer(Modifier.width(6.dp))
                TileFace(game.indicator, tileW, tileH)
                Spacer(Modifier.width(18.dp))
                Text("余牌 ${game.wall.size}", color = TableColors.Accent, fontSize = 13.sp)
            }
            Spacer(Modifier.height(6.dp))
            Row(horizontalArrangement = Arrangement.spacedBy(14.dp)) {
                for (s in 0 until 4) {
                    val active = game.turn == s && !game.over
                    Text(
                        text = "${seatName(s)} ${game.player(s).score}",
                        color = if (active) TableColors.Gold else TableColors.Accent.copy(alpha = 0.75f),
                        fontSize = 12.sp,
                        fontWeight = if (active) FontWeight.Bold else FontWeight.Normal,
                    )
                }
            }
        }
    }
}

@Composable
private fun OpponentColumn(game: Game, seat: Int, tileW: Dp, tileH: Dp, modifier: Modifier = Modifier) {
    val p = game.player(seat)
    val active = game.turn == seat && !game.over
    Column(
        modifier = modifier.padding(top = 40.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        SeatLabel(seat, p.score, active)
        Row(horizontalArrangement = Arrangement.spacedBy(1.dp)) {
            repeat(p.hand.size) { TileBack(tileW * 0.62f, tileH * 0.62f) }
        }
        MeldsRow(p.melds, tileW * 0.62f, tileH * 0.62f)
        DiscardRow(p.discards, tileW * 0.66f, tileH * 0.66f)
    }
}

@Composable
private fun OpponentSide(game: Game, seat: Int, tileW: Dp, tileH: Dp, modifier: Modifier = Modifier) {
    val p = game.player(seat)
    val active = game.turn == seat && !game.over
    Column(
        modifier = modifier.padding(horizontal = 4.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        SeatLabel(seat, p.score, active)
        Column(verticalArrangement = Arrangement.spacedBy(1.dp)) {
            repeat(p.hand.size) { TileBackRotated(tileW * 0.66f, tileH * 0.66f) }
        }
        MeldsRow(p.melds, tileW * 0.6f, tileH * 0.6f)
        DiscardRow(p.discards, tileW * 0.62f, tileH * 0.62f)
    }
}

@Composable
private fun SeatLabel(seat: Int, score: Int, active: Boolean) {
    Surface(
        shape = RoundedCornerShape(20),
        color = if (active) TableColors.ActiveSeat else TableColors.Felt,
        modifier = Modifier.padding(2.dp),
    ) {
        Text(
            text = "${seatName(seat)}  $score",
            color = TableColors.Accent,
            fontSize = 11.sp,
            modifier = Modifier.padding(horizontal = 8.dp, vertical = 2.dp),
        )
    }
}

@Composable
private fun MeldsRow(melds: List<Meld>, tileW: Dp, tileH: Dp) {
    if (melds.isEmpty()) return
    Column(verticalArrangement = Arrangement.spacedBy(1.dp), modifier = Modifier.padding(top = 2.dp)) {
        for (m in melds) {
            Row(horizontalArrangement = Arrangement.spacedBy(1.dp)) {
                for (t in m.tiles) TileFace(t, tileW, tileH)
                Spacer(Modifier.width(4.dp))
            }
        }
    }
}

@Composable
private fun DiscardRow(tiles: List<Int>, tileW: Dp, tileH: Dp, modifier: Modifier = Modifier) {
    if (tiles.isEmpty()) return
    Column(modifier = modifier.padding(top = 3.dp), verticalArrangement = Arrangement.spacedBy(1.dp)) {
        for (chunk in tiles.chunked(6)) {
            Row(horizontalArrangement = Arrangement.spacedBy(1.dp)) {
                for (t in chunk) TileFace(t, tileW, tileH)
            }
        }
    }
}

@Composable
private fun HumanArea(
    controller: GameController,
    game: Game,
    prompt: Prompt,
    tileW: Dp,
    tileH: Dp,
    modifier: Modifier = Modifier,
) {
    val p = game.player(HUMAN)
    val canDiscard = prompt == Prompt.TURN
    Column(
        modifier = modifier.padding(bottom = 6.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        Row(
            horizontalArrangement = Arrangement.spacedBy(3.dp),
            verticalAlignment = Alignment.Bottom,
        ) {
            for (t in p.hand) {
                TileFace(
                    index = t,
                    width = tileW,
                    height = tileH,
                    modifier = Modifier
                        .alpha(if (canDiscard) 1f else 0.92f)
                        .clickable(enabled = canDiscard) { controller.submit(HumanMove.Discard(t)) },
                )
            }
            if (p.melds.isNotEmpty()) {
                Spacer(Modifier.width(14.dp))
                for (m in p.melds) {
                    Row(horizontalArrangement = Arrangement.spacedBy(1.dp), verticalAlignment = Alignment.Bottom) {
                        for (t in m.tiles) TileFace(t, tileW * 0.8f, tileH * 0.8f)
                        Spacer(Modifier.width(4.dp))
                    }
                }
            }
        }
    }
}

@Composable
private fun ActionBar(controller: GameController, game: Game, prompt: Prompt, modifier: Modifier = Modifier) {
    val p = game.player(HUMAN)
    Surface(
        modifier = modifier.widthIn(min = 200.dp),
        shape = MaterialTheme.shapes.large,
        color = TableColors.FeltDark.copy(alpha = 0.92f),
    ) {
        Column(
            modifier = Modifier.padding(14.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            if (prompt == Prompt.TURN) {
                Text("轮到你出牌", color = TableColors.Accent, fontSize = 14.sp)
                Spacer(Modifier.height(8.dp))
                Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                    if (game.checkSelfWin(HUMAN) != null) {
                        ActionButton("胡", TableColors.Gold, true) { controller.submit(HumanMove.SelfWin) }
                    }
                    for (g in game.selfGangOptions(HUMAN)) {
                        val label = if (g.type == MeldType.AN_GANG) "暗杠" else "补杠"
                        ActionButton(label, TableColors.ActiveSeat, true) { controller.submit(HumanMove.Gang(g)) }
                    }
                }
            } else {
                val cand = game.pending?.candidates?.firstOrNull { it.seat == HUMAN }
                Text("是否响应？", color = TableColors.Accent, fontSize = 14.sp)
                Spacer(Modifier.height(8.dp))
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    if (cand != null && "win" in cand.actions) {
                        ActionButton("胡", TableColors.Gold, true) { controller.submit(HumanMove.Claim(ClaimDecision.Win)) }
                    }
                    if (cand != null && "gang" in cand.actions) {
                        ActionButton("杠", TableColors.ActiveSeat, true) { controller.submit(HumanMove.Claim(ClaimDecision.Gang)) }
                    }
                    if (cand != null && "peng" in cand.actions) {
                        ActionButton("碰", TableColors.ActiveSeat, true) { controller.submit(HumanMove.Claim(ClaimDecision.Peng)) }
                    }
                    if (cand != null && "chi" in cand.actions) {
                        for (opt in cand.chiOptions) {
                            ActionButton(
                                "吃 " + tileName(opt[0]) + "+" + tileName(opt[1]),
                                TableColors.RailLight,
                                true,
                            ) { controller.submit(HumanMove.Claim(ClaimDecision.Chi(opt))) }
                        }
                    }
                    ActionButton("过", TableColors.Rail, true) { controller.submit(HumanMove.Claim(null)) }
                }
            }
        }
    }
}

@Composable
private fun ActionButton(label: String, color: Color, enabled: Boolean, onClick: () -> Unit) {
    Button(
        onClick = onClick,
        enabled = enabled,
        colors = ButtonDefaults.buttonColors(containerColor = color, contentColor = TableColors.FeltDark),
    ) {
        Text(label, fontWeight = FontWeight.Bold)
    }
}

@Composable
private fun ResultOverlay(game: Game, onRestart: () -> Unit, onExit: () -> Unit) {
    val res = game.result
    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Color.Black.copy(alpha = 0.55f))
            .clickable(enabled = false) {},
        contentAlignment = Alignment.Center,
    ) {
        Surface(shape = MaterialTheme.shapes.large, color = TableColors.Felt, modifier = Modifier.widthIn(min = 280.dp)) {
            Column(
                modifier = Modifier.padding(24.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
            ) {
                val title = when {
                    res == null -> "本局结束"
                    res.drawnGame -> "流局（连庄）"
                    else -> "${seatName(res.winner)} 胡牌！"
                }
                Text(title, color = TableColors.Gold, fontSize = 26.sp, fontWeight = FontWeight.Bold)
                Spacer(Modifier.height(10.dp))
                if (res != null && !res.drawnGame) {
                    val how = if (res.selfDraw) "自摸" else "捉冲"
                    val pats = res.win.patterns.filter { it != "自摸" }.joinToString(" · ")
                    Text("$how  ${res.win.multiplier} 倍", color = TableColors.Accent, fontSize = 16.sp)
                    if (pats.isNotEmpty()) {
                        Text(pats, color = TableColors.Accent.copy(alpha = 0.8f), fontSize = 13.sp)
                    }
                }
                val cp = res?.contractPair
                if (cp != null) {
                    Text(
                        "承包生效：${seatName(cp.first)} ↔ ${seatName(cp.second)}",
                        color = TableColors.RailLight,
                        fontSize = 12.sp,
                    )
                }
                Spacer(Modifier.height(14.dp))
                Column(horizontalAlignment = Alignment.Start) {
                    for (s in 0 until 4) {
                        Text(
                            "${seatName(s)}：${game.player(s).score}",
                            color = TableColors.Accent,
                            fontSize = 14.sp,
                        )
                    }
                }
                Spacer(Modifier.height(18.dp))
                Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                    ActionButton("再来一局", TableColors.Gold, true, onRestart)
                    OutlinedButton(onClick = onExit) { Text("返回主菜单", color = TableColors.Accent) }
                }
            }
        }
    }
}
