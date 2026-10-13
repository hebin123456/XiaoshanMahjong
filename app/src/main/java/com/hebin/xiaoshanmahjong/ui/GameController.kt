package com.hebin.xiaoshanmahjong.ui

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.hebin.xiaoshanmahjong.game.AiLevel
import com.hebin.xiaoshanmahjong.game.ClaimDecision
import com.hebin.xiaoshanmahjong.game.GangOption
import com.hebin.xiaoshanmahjong.game.Game
import com.hebin.xiaoshanmahjong.game.Phase
import com.hebin.xiaoshanmahjong.game.RuleSet
import com.hebin.xiaoshanmahjong.game.chooseClaim
import com.hebin.xiaoshanmahjong.game.chooseDiscard
import com.hebin.xiaoshanmahjong.game.chooseSelfGang
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.delay

/** 人类玩家固定为 0 号位（下方）。 */
const val HUMAN = 0

enum class Prompt { NONE, TURN, CLAIM }

sealed interface HumanMove {
    data class Discard(val tile: Int) : HumanMove
    data object SelfWin : HumanMove
    data class Gang(val option: GangOption) : HumanMove
    data class Claim(val decision: ClaimDecision?) : HumanMove
}

/**
 * 对局控制器：驱动引擎走完整局，人类回合时挂起等待输入。
 * 复用 Compose 状态，让界面能随引擎推进自动重绘。
 */
class GameController(val rules: RuleSet) {
    var game: Game by mutableStateOf(Game(rules, System.nanoTime()))
        private set

    /** 版本号：每次引擎变化后自增，用来触发重组。 */
    var rev by mutableIntStateOf(0)
        private set

    var prompt by mutableStateOf(Prompt.NONE)
        private set

    var lastDiscardSeat by mutableIntStateOf(-1)
        private set

    private var moveDeferred: CompletableDeferred<HumanMove>? = null

    fun submit(move: HumanMove) {
        moveDeferred?.complete(move)
    }

    private suspend fun awaitMove(p: Prompt): HumanMove {
        prompt = p
        rev++
        val deferred = CompletableDeferred<HumanMove>()
        moveDeferred = deferred
        val move = deferred.await()
        moveDeferred = null
        prompt = Prompt.NONE
        rev++
        return move
    }

    private fun aiDelayMs(): Long = when (rules.aiLevel) {
        AiLevel.EASY -> 420L
        AiLevel.NORMAL -> 620L
        AiLevel.HARD -> 820L
    }

    suspend fun run() {
        rev++
        while (!game.over) {
            when (game.phase) {
                Phase.DRAW -> {
                    delay(160)
                    game.drawFor(game.turn)
                    rev++
                }

                Phase.DISCARD -> {
                    val seat = game.turn
                    if (seat == HUMAN) {
                        val move = awaitMove(Prompt.TURN)
                        when (move) {
                            is HumanMove.SelfWin -> game.finishSelfDraw(HUMAN)
                            is HumanMove.Gang -> game.doSelfGang(HUMAN, move.option.tile, move.option.type)
                            is HumanMove.Discard -> {
                                game.discard(HUMAN, move.tile)
                                lastDiscardSeat = HUMAN
                            }
                            else -> {}
                        }
                    } else {
                        delay(aiDelayMs())
                        if (game.checkSelfWin(seat) != null) {
                            game.finishSelfDraw(seat)
                        } else {
                            val gangs = game.selfGangOptions(seat)
                            val gang = chooseSelfGang(game, seat, gangs)
                            if (gang != null) {
                                game.doSelfGang(seat, gang.tile, gang.type)
                            } else {
                                game.discard(seat, chooseDiscard(game, seat))
                                lastDiscardSeat = seat
                            }
                        }
                    }
                    rev++
                }

                Phase.CLAIM -> {
                    val pd = game.pending!!
                    for (cand in pd.candidates) {
                        if (cand.seat != HUMAN) {
                            game.submitClaim(cand.seat, chooseClaim(game, cand.seat, cand))
                        }
                    }
                    val humanCand = pd.candidates.firstOrNull { it.seat == HUMAN }
                    if (humanCand != null) {
                        val move = awaitMove(Prompt.CLAIM)
                        game.submitClaim(HUMAN, (move as? HumanMove.Claim)?.decision)
                    }
                    delay(240)
                    game.resolveClaims()
                    rev++
                }

                Phase.OVER -> {
                    rev++
                    break
                }
            }
        }
        rev++
    }
}
