package com.hebin.xiaoshanmahjong.ui

import androidx.compose.animation.core.Animatable
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.rotate
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.hebin.xiaoshanmahjong.game.ClaimDecision
import com.hebin.xiaoshanmahjong.game.Game
import com.hebin.xiaoshanmahjong.game.Meld
import com.hebin.xiaoshanmahjong.game.MeldType
import com.hebin.xiaoshanmahjong.game.RuleSet
import com.hebin.xiaoshanmahjong.game.tileName

/** 0=你（下）1=下家（右）2=对家（上）3=上家（左） */
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

    val game = controller.game
    val prompt = controller.prompt

    BoxWithConstraints(
        modifier = Modifier
            .fillMaxSize()
            .background(TableColors.FeltDark),
    ) {
        val h = maxHeight
        val w = maxWidth

        val topBarH = 36.dp
        val actionH = 56.dp
        // 主体可用高度
        val budget = h - 12.dp - topBarH - actionH
        val sideTile = ((budget - 26.dp) / 14f).coerceIn(12.dp, 20.dp)
        val sidePanelW = sideTile * 1.33f + 8.dp
        val centerColW = (w - 12.dp) - sidePanelW * 2 - 6.dp

        val backTileH = (h * 0.056f).coerceIn(16.dp, 26.dp)
        val discardTileH = (h * 0.062f).coerceIn(18.dp, 30.dp)

        // 手牌尺寸：先按宽度均分，再受高度预算约束，保证不溢出
        val human = game.player(HUMAN)
        val handTotal = (human.hand.size + human.melds.sumOf { it.tiles.size }).coerceAtLeast(13)
        val widthHandH = ((centerColW - 28.dp) / handTotal) * 4f / 3f
        val fixedTop = 18.dp + backTileH + discardTileH
        val maxHandH = budget - fixedTop - discardTileH - 46.dp - 8.dp
        val handTileH = minOf(widthHandH, maxHandH, (h * 0.26f).coerceIn(56.dp, 104.dp)).coerceAtLeast(42.dp)

        // 摸牌动画：0=已落位
        val drawAnim = remember { Animatable(0f) }
        val drawnTile = game.lastDrawnTile
        val drawnVisible = game.turn == HUMAN && !game.over &&
            drawnTile >= 0 && human.hand.count { it == drawnTile } > 0
        LaunchedEffect(drawnTile, game.turn, human.hand.size) {
            if (drawnVisible) {
                drawAnim.snapTo(1f)
                drawAnim.animateTo(0f, animationSpec = tween(320))
            }
        }

        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(horizontal = 6.dp, vertical = 6.dp),
        ) {
            TopBar(rules, game, onExit, topBarH)

            Row(modifier = Modifier.fillMaxWidth().weight(1f)) {
                SideHand(game, 3, sideTile, Modifier.fillMaxHeight())

                Column(
                    modifier = Modifier.weight(1f).fillMaxHeight(),
                    horizontalAlignment = Alignment.CenterHorizontally,
                ) {
                    // 对家（上）
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        SeatHeader(2, game)
                        Spacer(Modifier.width(6.dp))
                        BacksRow(game.player(2).hand.size, backTileH)
                        Spacer(Modifier.width(10.dp))
                        MeldsRow(game.player(2).melds, backTileH)
                    }
                    DiscardArea(
                        tiles = game.player(2).discards,
                        tileH = discardTileH,
                        game = game,
                        seat = 2,
                        maxPerRow = 20,
                        modifier = Modifier.padding(top = 2.dp),
                    )

                    // 中央带：上家弃牌 | 财神 | 下家弃牌
                    Row(modifier = Modifier.fillMaxWidth().weight(1f)) {
                        DiscardArea(
                            tiles = game.player(3).discards,
                            tileH = discardTileH,
                            game = game,
                            seat = 3,
                            maxPerRow = 8,
                            modifier = Modifier.weight(1f).fillMaxHeight(),
                        )
                        CenterPanel(game, Modifier.align(Alignment.CenterVertically))
                        DiscardArea(
                            tiles = game.player(1).discards,
                            tileH = discardTileH,
                            game = game,
                            seat = 1,
                            maxPerRow = 8,
                            modifier = Modifier.weight(1f).fillMaxHeight(),
                        )
                    }

                    // 我方弃牌
                    DiscardArea(
                        tiles = human.discards,
                        tileH = discardTileH,
                        game = game,
                        seat = 0,
                        maxPerRow = 24,
                        modifier = Modifier.padding(top = 2.dp),
                    )

                    // 我的手牌
                    MyHand(controller, game, prompt, handTileH, drawAnim.value, drawnVisible)
                }

                SideHand(game, 1, sideTile, Modifier.fillMaxHeight())
            }

            ActionBar(controller, game, prompt, actionH)
        }

        // 摸牌时的手部动画
        if (drawnVisible) {
            Canvas(Modifier.fillMaxSize()) {
                drawTableHand(
                    center = Offset(size.width * 0.90f, size.height * 0.86f),
                    size = size.height * 0.11f,
                    alpha = drawAnim.value,
                )
            }
        }

        if (game.over) ResultOverlay(game, onRestart, onExit)
    }
}

private fun ruleSummary(rules: RuleSet): String {
    val rush = if (rules.allowRush) "允许捉冲" else "自摸专属"
    val bao = if (rules.mustBaoHead) "财神须爆头" else "财神随意"
    val chi = if (rules.allowChiThree) "允许吃三摊" else "最多两摊"
    return "${rush} · $bao · $chi"
}

/* ---------------------------------- 顶栏 ---------------------------------- */

@Composable
private fun TopBar(rules: RuleSet, game: Game, onExit: () -> Unit, height: Dp) {
    Row(
        modifier = Modifier.fillMaxWidth().height(height),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        TextButton(
            onClick = onExit,
            contentPadding = PaddingValues(horizontal = 10.dp, vertical = 0.dp),
        ) { Text("‹ 返回", color = TableColors.Accent, fontSize = 13.sp) }

        Text("萧山麻将", color = TableColors.Gold, fontSize = 15.sp, fontWeight = FontWeight.Bold)
        Spacer(Modifier.width(8.dp))
        Text(ruleSummary(rules), color = TableColors.Accent.copy(alpha = 0.5f), fontSize = 11.sp)

        Spacer(Modifier.weight(1f))

        for (s in 0 until 4) {
            ScoreChip(s, game)
            Spacer(Modifier.width(5.dp))
        }
        Text("余牌 ${game.wall.size}", color = TableColors.Accent.copy(alpha = 0.85f), fontSize = 12.sp)
    }
}

@Composable
private fun ScoreChip(seat: Int, game: Game) {
    val active = game.turn == seat && !game.over
    Surface(
        shape = RoundedCornerShape(20.dp),
        color = if (active) TableColors.ActiveSeat else TableColors.Felt.copy(alpha = 0.7f),
    ) {
        Row(
            modifier = Modifier.padding(horizontal = 9.dp, vertical = 3.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Text(seatName(seat), color = TableColors.Accent.copy(alpha = 0.8f), fontSize = 11.sp)
            Spacer(Modifier.width(5.dp))
            Text(
                text = "${game.player(seat).score}",
                color = if (active) Color.White else TableColors.Accent,
                fontSize = 11.sp,
                fontWeight = if (active) FontWeight.Bold else FontWeight.Normal,
            )
        }
    }
}

@Composable
private fun SeatHeader(seat: Int, game: Game, modifier: Modifier = Modifier) {
    val active = game.turn == seat && !game.over
    Surface(
        modifier = modifier,
        shape = RoundedCornerShape(20.dp),
        color = if (active) TableColors.ActiveSeat else TableColors.Felt.copy(alpha = 0.7f),
    ) {
        Row(
            modifier = Modifier.padding(horizontal = 8.dp, vertical = 2.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            if (game.dealer == seat) {
                Text("庄", color = TableColors.Gold, fontSize = 10.sp, fontWeight = FontWeight.Bold)
                Spacer(Modifier.width(4.dp))
            }
            Text(
                text = "${seatName(seat)} ${game.player(seat).score}",
                color = TableColors.Accent,
                fontSize = 11.sp,
                fontWeight = if (active) FontWeight.Bold else FontWeight.Normal,
            )
        }
    }
}

/* ---------------------------------- 对手 ---------------------------------- */

@Composable
private fun BacksRow(count: Int, tileH: Dp) {
    val w = tileH * 0.72f
    Row(horizontalArrangement = Arrangement.spacedBy(1.dp)) {
        repeat(count) { TileBack(w, tileH) }
    }
}

@Composable
private fun MeldsRow(melds: List<Meld>, tileH: Dp) {
    if (melds.isEmpty()) return
    val w = tileH * 0.72f
    Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
        for (m in melds) {
            Row(horizontalArrangement = Arrangement.spacedBy(1.dp)) {
                for (t in m.tiles) TileFace(t, w, tileH)
            }
        }
    }
}

/** 左右两家：一列竖排的旋转牌（牌背 + 亮牌）。 */
@Composable
private fun SideHand(game: Game, seat: Int, tile: Dp, modifier: Modifier = Modifier) {
    val p = game.player(seat)
    Column(
        modifier = modifier
            .width(tile * 1.33f + 8.dp)
            .verticalScroll(rememberScrollState())
            .padding(vertical = 2.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        SeatHeader(seat, game, Modifier.padding(bottom = 3.dp))
        repeat(p.hand.size) { RotatedBack(tile) }
        for (m in p.melds) for (t in m.tiles) RotatedFace(t, tile)
    }
}

@Composable
private fun RotatedBack(tile: Dp) {
    Box(Modifier.size(tile * 1.33f, tile), contentAlignment = Alignment.Center) {
        TileBack(tile, tile * 1.33f, Modifier.rotate(90f))
    }
}

@Composable
private fun RotatedFace(index: Int, tile: Dp) {
    Box(Modifier.size(tile * 1.33f, tile), contentAlignment = Alignment.Center) {
        TileFace(index, tile, tile * 1.33f, Modifier.rotate(90f))
    }
}

/* ---------------------------------- 弃牌 ---------------------------------- */

@OptIn(ExperimentalLayoutApi::class)
@Composable
private fun DiscardArea(
    tiles: List<Int>,
    tileH: Dp,
    game: Game,
    seat: Int,
    maxPerRow: Int,
    modifier: Modifier = Modifier,
) {
    if (tiles.isEmpty()) return
    val w = tileH * 0.72f
    Box(modifier = modifier.padding(1.dp), contentAlignment = Alignment.Center) {
        FlowRow(
            horizontalArrangement = Arrangement.spacedBy(2.dp),
            verticalArrangement = Arrangement.spacedBy(2.dp),
            maxItemsInEachRow = maxPerRow,
        ) {
            tiles.forEachIndexed { i, t ->
                val isLast = seat == game.lastDiscardFrom && i == tiles.lastIndex
                DiscardTile(t, w, tileH, isLast)
            }
        }
    }
}

@Composable
private fun DiscardTile(t: Int, w: Dp, h: Dp, highlight: Boolean) {
    Box(Modifier.size(w, h)) {
        TileFace(t, w, h)
        if (highlight) {
            Box(
                Modifier
                    .size(w, h)
                    .border(2.dp, TableColors.Gold, RoundedCornerShape(4.dp)),
            )
        }
    }
}

/* ---------------------------------- 中央 ---------------------------------- */

@Composable
private fun CenterPanel(game: Game, modifier: Modifier = Modifier) {
    Surface(
        modifier = modifier,
        shape = RoundedCornerShape(14.dp),
        color = TableColors.Felt.copy(alpha = 0.9f),
    ) {
        Column(
            modifier = Modifier.padding(horizontal = 14.dp, vertical = 8.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            Text("财神", color = TableColors.Gold, fontSize = 11.sp)
            Spacer(Modifier.height(4.dp))
            if (game.indicator >= 0) TileFace(game.indicator, 34.dp, 45.dp)
            Spacer(Modifier.height(4.dp))
            Text("余 ${game.wall.size}", color = TableColors.Accent.copy(alpha = 0.8f), fontSize = 11.sp)
        }
    }
}

/* ---------------------------------- 我的手牌 ---------------------------------- */

@Composable
private fun MyHand(
    controller: GameController,
    game: Game,
    prompt: Prompt,
    tileH: Dp,
    animValue: Float,
    drawnVisible: Boolean,
) {
    val tileW = tileH * 0.75f
    val p = game.player(HUMAN)
    val canDiscard = prompt == Prompt.TURN
    val drawn = game.lastDrawnTile

    // 把手牌拆成「已理好的牌」+「刚摸到的那张」
    val sorted = remember(p.hand.toList(), drawn, drawnVisible) {
        val list = ArrayList(p.hand)
        if (drawnVisible) list.remove(drawn)
        list.sort()
        list
    }

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(top = 3.dp, bottom = 2.dp),
        horizontalArrangement = Arrangement.Center,
        verticalAlignment = Alignment.Bottom,
    ) {
        for (t in sorted) {
            TileFace(
                index = t,
                width = tileW,
                height = tileH,
                modifier = Modifier.clickable(enabled = canDiscard) {
                    controller.submit(HumanMove.Discard(t))
                },
            )
        }

        if (p.melds.isNotEmpty()) {
            Spacer(Modifier.width(12.dp))
            for (m in p.melds) {
                for (t in m.tiles) {
                    TileFace(
                        index = t,
                        width = tileW * 0.88f,
                        height = tileH * 0.88f,
                        modifier = Modifier.align(Alignment.Bottom),
                    )
                }
                Spacer(Modifier.width(6.dp))
            }
        }

        if (drawnVisible) {
            Spacer(Modifier.width(14.dp))
            TileFace(
                index = drawn,
                width = tileW,
                height = tileH,
                modifier = Modifier
                    .offset(x = (animValue * 34f).dp, y = (-animValue * 16f).dp)
                    .clickable(enabled = canDiscard) {
                        controller.submit(HumanMove.Discard(drawn))
                    },
            )
        }
    }
}

/* ---------------------------------- 操作栏 ---------------------------------- */

@Composable
private fun ActionBar(controller: GameController, game: Game, prompt: Prompt, height: Dp) {
    Surface(
        modifier = Modifier.fillMaxWidth().height(height),
        color = TableColors.Felt.copy(alpha = 0.95f),
        shape = RoundedCornerShape(12.dp),
    ) {
        Row(
            modifier = Modifier.fillMaxSize().padding(horizontal = 14.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            when (prompt) {
                Prompt.TURN -> {
                    Text("轮到你出牌 · 点击一张牌打出", color = TableColors.Accent, fontSize = 13.sp)
                    Spacer(Modifier.weight(1f))
                    Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                        if (game.checkSelfWin(HUMAN) != null) {
                            ActionButton("胡", TableColors.Gold) { controller.submit(HumanMove.SelfWin) }
                        }
                        for (g in game.selfGangOptions(HUMAN)) {
                            val label = if (g.type == MeldType.AN_GANG) "暗杠" else "补杠"
                            ActionButton(label, TableColors.ActiveSeat) { controller.submit(HumanMove.Gang(g)) }
                        }
                    }
                }

                Prompt.CLAIM -> {
                    val cand = game.pending?.candidates?.firstOrNull { it.seat == HUMAN }
                    Text("可以响应：", color = TableColors.Accent, fontSize = 13.sp)
                    Spacer(Modifier.width(10.dp))
                    Row(
                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                        verticalAlignment = Alignment.CenterVertically,
                    ) {
                        if (cand != null && "win" in cand.actions) {
                            ActionButton("胡", TableColors.Gold) { controller.submit(HumanMove.Claim(ClaimDecision.Win)) }
                        }
                        if (cand != null && "gang" in cand.actions) {
                            ActionButton("杠", TableColors.ActiveSeat) { controller.submit(HumanMove.Claim(ClaimDecision.Gang)) }
                        }
                        if (cand != null && "peng" in cand.actions) {
                            ActionButton("碰", TableColors.ActiveSeat) { controller.submit(HumanMove.Claim(ClaimDecision.Peng)) }
                        }
                        if (cand != null && "chi" in cand.actions) {
                            for (opt in cand.chiOptions) {
                                ActionButton(
                                    "吃 ${tileName(opt[0])}+${tileName(opt[1])}",
                                    TableColors.RailLight,
                                ) { controller.submit(HumanMove.Claim(ClaimDecision.Chi(opt))) }
                            }
                        }
                        ActionButton("过", TableColors.Rail) { controller.submit(HumanMove.Claim(null)) }
                    }
                    Spacer(Modifier.weight(1f))
                    val pd = game.pending
                    if (pd != null) {
                        Text(
                            "待应牌：${tileName(pd.tile)}（${seatName(pd.from)}打出）",
                            color = TableColors.Accent.copy(alpha = 0.6f),
                            fontSize = 12.sp,
                        )
                    }
                }

                Prompt.NONE -> {
                    Text(
                        text = if (game.over) "本局结束" else "${seatName(game.turn)} 思考中…",
                        color = TableColors.Accent.copy(alpha = 0.8f),
                        fontSize = 13.sp,
                    )
                    Spacer(Modifier.weight(1f))
                    Text(
                        text = "财神：白板百搭",
                        color = TableColors.Accent.copy(alpha = 0.4f),
                        fontSize = 11.sp,
                    )
                }
            }
        }
    }
}

@Composable
private fun ActionButton(label: String, color: Color, onClick: () -> Unit) {
    Button(
        onClick = onClick,
        colors = ButtonDefaults.buttonColors(
            containerColor = color,
            contentColor = TableColors.FeltDark,
        ),
        contentPadding = PaddingValues(horizontal = 16.dp, vertical = 6.dp),
    ) {
        Text(label, fontWeight = FontWeight.Bold, fontSize = 14.sp)
    }
}

/* ---------------------------------- 结果 ---------------------------------- */

@Composable
private fun ResultOverlay(game: Game, onRestart: () -> Unit, onExit: () -> Unit) {
    val res = game.result
    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Color.Black.copy(alpha = 0.6f))
            .clickable(enabled = false) {},
        contentAlignment = Alignment.Center,
    ) {
        Surface(
            shape = RoundedCornerShape(18.dp),
            color = TableColors.Felt,
            modifier = Modifier.padding(24.dp),
        ) {
            Column(
                modifier = Modifier.padding(horizontal = 28.dp, vertical = 20.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
            ) {
                val title = when {
                    res == null -> "本局结束"
                    res.drawnGame -> "流局（连庄）"
                    else -> "${seatName(res.winner)} 胡牌！"
                }
                Text(title, color = TableColors.Gold, fontSize = 26.sp, fontWeight = FontWeight.Bold)

                if (res != null && !res.drawnGame) {
                    Spacer(Modifier.height(8.dp))
                    val how = if (res.selfDraw) "自摸" else "捉冲"
                    Text("$how · ${res.win.multiplier} 倍", color = TableColors.Accent, fontSize = 15.sp)
                    val pats = res.win.patterns.filter { it != "自摸" }
                    if (pats.isNotEmpty()) {
                        Spacer(Modifier.height(3.dp))
                        Text(pats.joinToString(" · "), color = TableColors.Accent.copy(alpha = 0.8f), fontSize = 13.sp)
                    }
                }

                val cp = res?.contractPair
                if (cp != null) {
                    Spacer(Modifier.height(4.dp))
                    Text(
                        "承包生效：${seatName(cp.first)} ↔ ${seatName(cp.second)}",
                        color = TableColors.RailLight,
                        fontSize = 12.sp,
                    )
                }

                Spacer(Modifier.height(14.dp))
                for (s in 0 until 4) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                    ) {
                        Text(seatName(s), color = TableColors.Accent, fontSize = 14.sp)
                        Text(
                            text = "${game.player(s).score}",
                            color = if (res != null && !res.drawnGame && res.winner == s) TableColors.Gold else TableColors.Accent,
                            fontSize = 14.sp,
                            fontWeight = FontWeight.Bold,
                        )
                    }
                }

                Spacer(Modifier.height(18.dp))
                Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                    ActionButton("再来一局", TableColors.Gold, onRestart)
                    OutlinedButton(onClick = onExit) { Text("返回主菜单", color = TableColors.Accent) }
                }
            }
        }
    }
}

/* -------------------------------- 手部图形 -------------------------------- */

private fun DrawScope.drawTableHand(center: Offset, size: Float, alpha: Float) {
    if (alpha <= 0.02f) return
    val skin = Color(0xFFF1CBA6).copy(alpha = alpha * 0.95f)
    val skinDark = Color(0xFFD8A87D).copy(alpha = alpha * 0.95f)
    val palmW = size * 0.62f
    val palmH = size * 0.58f

    // 四指
    val fingerW = palmW * 0.20f
    val fingerH = size * 0.5f
    for (i in 0 until 4) {
        val x = center.x - palmW / 2f + palmW * 0.05f + i * (palmW * 0.25f)
        drawRoundRect(
            color = skin,
            topLeft = Offset(x, center.y - fingerH * 0.95f),
            size = Size(fingerW, fingerH),
            cornerRadius = CornerRadius(fingerW / 2f),
        )
    }
    // 掌心
    drawRoundRect(
        color = skin,
        topLeft = Offset(center.x - palmW / 2f, center.y - palmH * 0.4f),
        size = Size(palmW, palmH),
        cornerRadius = CornerRadius(palmW * 0.34f),
    )
    // 拇指
    drawRoundRect(
        color = skinDark,
        topLeft = Offset(center.x + palmW * 0.24f, center.y - palmH * 0.15f),
        size = Size(fingerW * 1.1f, size * 0.34f),
        cornerRadius = CornerRadius(fingerW / 2f),
    )
    // 掌心描边
    drawRoundRect(
        color = skinDark,
        topLeft = Offset(center.x - palmW / 2f, center.y - palmH * 0.4f),
        size = Size(palmW, palmH),
        cornerRadius = CornerRadius(palmW * 0.34f),
        style = Stroke(width = 1.5f),
    )
}
