package com.hebin.xiaoshanmahjong.ui

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color

object TableColors {
    val Felt = Color(0xFF0B3D2E)
    val FeltDark = Color(0xFF082B20)
    val Rail = Color(0xFF6B3E22)
    val RailLight = Color(0xFF8A5330)
    val Accent = Color(0xFFE8C87A)
    val Gold = Color(0xFFD9A94A)
    val Tile = Color(0xFFF7F3E8)
    val ActiveSeat = Color(0xFF3E8E6E)
}

private val scheme = darkColorScheme(
    primary = Color(0xFF3E8E6E),
    onPrimary = Color.White,
    secondary = TableColors.Gold,
    background = TableColors.FeltDark,
    surface = TableColors.Felt,
    onSurface = Color(0xFFEAF3EE),
)

@Composable
fun XiaoshanTheme(content: @Composable () -> Unit) {
    MaterialTheme(colorScheme = scheme, content = content)
}
