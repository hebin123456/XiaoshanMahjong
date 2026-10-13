package com.hebin.xiaoshanmahjong

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.platform.LocalContext
import com.hebin.xiaoshanmahjong.game.RuleSet
import com.hebin.xiaoshanmahjong.ui.GameScreen
import com.hebin.xiaoshanmahjong.ui.MenuScreen
import com.hebin.xiaoshanmahjong.ui.SettingsStore
import com.hebin.xiaoshanmahjong.ui.XiaoshanTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent { AppRoot() }
    }
}

@Composable
fun AppRoot() {
    val context = LocalContext.current
    var rules by remember { mutableStateOf(SettingsStore.load(context)) }
    var inGame by remember { mutableStateOf(false) }
    var gameId by remember { mutableIntStateOf(0) }

    XiaoshanTheme {
        if (inGame) {
            GameScreen(
                rules = rules,
                onExit = { inGame = false },
                onRestart = { gameId++ },
                key = gameId,
            )
        } else {
            MenuScreen(
                rules = rules,
                onRulesChange = {
                    rules = it
                    SettingsStore.save(context, it)
                },
                onStart = {
                    gameId++
                    inGame = true
                },
            )
        }
    }
}
