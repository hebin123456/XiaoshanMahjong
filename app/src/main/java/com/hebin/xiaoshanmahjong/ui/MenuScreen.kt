package com.hebin.xiaoshanmahjong.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Divider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.hebin.xiaoshanmahjong.game.AiLevel
import com.hebin.xiaoshanmahjong.game.RuleSet

fun aiLevelName(level: AiLevel): String = when (level) {
    AiLevel.EASY -> "入门"
    AiLevel.NORMAL -> "普通"
    AiLevel.HARD -> "高手"
}

@Composable
fun MenuScreen(rules: RuleSet, onRulesChange: (RuleSet) -> Unit, onStart: () -> Unit) {
    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Brush.verticalGradient(listOf(TableColors.FeltDark, TableColors.Felt))),
        contentAlignment = Alignment.Center,
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth(0.9f)
                .verticalScroll(rememberScrollState())
                .padding(20.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            Text(
                text = "萧山麻将",
                color = TableColors.Accent,
                fontSize = 40.sp,
                fontWeight = FontWeight.Bold,
            )
            Text(
                text = "你 vs 3 个电脑 · 财神百搭 · 番型计分",
                color = TableColors.Accent.copy(alpha = 0.7f),
                fontSize = 15.sp,
            )

            Spacer(Modifier.height(24.dp))

            SettingCard(title = "AI 强度") {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(10.dp),
                ) {
                    AiLevel.values().forEach { level ->
                        val selected = rules.aiLevel == level
                        Button(
                            onClick = { onRulesChange(rules.copy(aiLevel = level)) },
                            modifier = Modifier.weight(1f),
                            colors = ButtonDefaults.buttonColors(
                                containerColor = if (selected) TableColors.Gold else TableColors.FeltDark,
                                contentColor = if (selected) TableColors.FeltDark else TableColors.Accent,
                            ),
                        ) {
                            Text(aiLevelName(level))
                        }
                    }
                }
            }

            Spacer(Modifier.height(12.dp))

            SettingCard(title = "规则") {
                SwitchRow(
                    label = "允许捉冲（点炮胡）",
                    hint = "关闭则为「自摸专属」，只能自摸",
                    checked = rules.allowRush,
                    onCheckedChange = { onRulesChange(rules.copy(allowRush = it)) },
                )
                Divider(color = TableColors.Accent.copy(alpha = 0.15f))
                SwitchRow(
                    label = "有财神必须爆头",
                    hint = "手中财神至少留 1 张做将牌，多余的可自由组合",
                    checked = rules.mustBaoHead,
                    onCheckedChange = { onRulesChange(rules.copy(mustBaoHead = it)) },
                )
                Divider(color = TableColors.Accent.copy(alpha = 0.15f))
                SwitchRow(
                    label = "允许吃三摊（承包）",
                    hint = "关闭：每家最多吃碰杠 2 摊；开启：第 3 摊触发承包绑定",
                    checked = rules.allowChiThree,
                    onCheckedChange = { onRulesChange(rules.copy(allowChiThree = it)) },
                )
            }

            Spacer(Modifier.height(24.dp))

            Button(
                onClick = onStart,
                modifier = Modifier
                    .fillMaxWidth()
                    .height(56.dp),
                colors = ButtonDefaults.buttonColors(
                    containerColor = TableColors.Gold,
                    contentColor = TableColors.FeltDark,
                ),
            ) {
                Text("开始对局", fontSize = 20.sp, fontWeight = FontWeight.Bold)
            }
        }
    }
}

@Composable
private fun SettingCard(title: String, content: @Composable () -> Unit) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .background(TableColors.Felt.copy(alpha = 0.7f), MaterialTheme.shapes.medium)
            .padding(16.dp),
    ) {
        Text(title, color = TableColors.Gold, fontWeight = FontWeight.Bold, fontSize = 16.sp)
        Spacer(Modifier.height(10.dp))
        content()
    }
}

@Composable
private fun SwitchRow(label: String, hint: String, checked: Boolean, onCheckedChange: (Boolean) -> Unit) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 8.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Column(Modifier.weight(1f)) {
            Text(label, color = TableColors.Accent, fontSize = 15.sp)
            Text(hint, color = TableColors.Accent.copy(alpha = 0.55f), fontSize = 12.sp)
        }
        Spacer(Modifier.width(12.dp))
        Switch(checked = checked, onCheckedChange = onCheckedChange)
    }
}
