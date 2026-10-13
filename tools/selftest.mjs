// Node 自测：牌面 / 胡牌判定 / 番型计分
// 运行：node tools/selftest.mjs
import assert from 'node:assert/strict';
import { makeWall, toCounts, KIND_COUNT, seededRng } from '../js/tiles.js';
import {
  isWinningHand, isSevenPairs, winningTiles, evaluateWin,
} from '../js/rules.js';
import { Game, PHASE, nextSeat } from '../js/engine.js';
import { chooseDiscard, chooseClaim, chooseSelfGang, structureScore } from '../js/ai.js';

let passed = 0;
function it(name, fn) {
  try {
    fn();
    passed++;
    console.log('  ✓ ' + name);
  } catch (e) {
    console.error('  ✗ ' + name);
    console.error('    ' + (e && e.message));
    process.exitCode = 1;
  }
}

console.log('牌面');
it('一副牌共 136 张，每种 4 张', () => {
  const wall = makeWall(seededRng(1));
  assert.equal(wall.length, 136);
  const counts = toCounts(wall);
  assert.equal(counts.length, KIND_COUNT);
  for (const c of counts) assert.equal(c, 4);
});

console.log('胡牌判定');
it('标准牌型可胡', () => {
  const hand = ['m1', 'm1', 'm1', 'm2', 'm3', 'm4', 's1', 's2', 's3', 'p5', 'p6', 'p7', 'z1', 'z1'];
  assert.equal(isWinningHand(hand), true);
});

it('缺一张不能胡', () => {
  const hand = ['m1', 'm1', 'm1', 'm2', 'm3', 'm4', 's1', 's2', 's3', 'p5', 'p6', 'p7', 'z1', 'z2'];
  assert.equal(isWinningHand(hand), false);
});

it('财神可补顺子', () => {
  const hand = ['m1', 'm2', 'm3', 'm4', 'm5', 'm6', 's1', 's2', 's3', 'p7', 'p8', 'z7', 'z1', 'z1'];
  assert.equal(isWinningHand(hand), true);
});

it('财神不能凭空变成字牌对子以外的错牌', () => {
  const hand = ['m1', 'm2', 'm4', 'm5', 'm7', 'm8', 's1', 's2', 's4', 's5', 'p1', 'p3', 'p5', 'z2'];
  assert.equal(isWinningHand(hand), false);
});

console.log('七对');
it('天然七对判定为清七对（未用财神）', () => {
  const hand = ['m1', 'm1', 'm2', 'm2', 'm3', 'm3', 's1', 's1', 's2', 's2', 'p1', 'p1', 'z1', 'z1'];
  const r = isSevenPairs(hand);
  assert.ok(r);
  assert.equal(r.usedJoker, false);
});

it('含财神的七对判定为用财神', () => {
  const hand = ['m1', 'm1', 'm2', 'm2', 'm3', 'm3', 's1', 's1', 's2', 's2', 'p1', 'p1', 'z1', 'z7'];
  const r = isSevenPairs(hand);
  assert.ok(r);
  assert.equal(r.usedJoker, true);
});

console.log('听口');
it('听 p6/p9 以及财神（白板）', () => {
  const hand = ['m1', 'm2', 'm3', 'm4', 'm5', 'm6', 's1', 's2', 's3', 'p7', 'p8', 'z1', 'z1'];
  const t = winningTiles(hand);
  assert.deepEqual(t.slice().sort(), ['p6', 'p9', 'z7']);
});

console.log('番型计分');
it('普通自摸 ×1', () => {
  const hand = ['m1', 'm1', 'm1', 'm2', 'm3', 'm4', 's1', 's2', 's3', 'p5', 'p6', 'p7', 'z1', 'z1'];
  const r = evaluateWin(hand, [], { selfDraw: true });
  assert.equal(r.win, true);
  assert.equal(r.multiplier, 1);
});

it('碰碰胡 ×2', () => {
  const hand = ['m1', 'm1', 'm1', 'm2', 'm2', 'm2', 's1', 's1', 's1', 'p5', 'p5', 'p5', 'z1', 'z1'];
  const r = evaluateWin(hand, [], { selfDraw: true });
  assert.equal(r.multiplier, 2);
  assert.ok(r.patterns.includes('碰碰胡'));
});

it('清一色 ×8', () => {
  const hand = ['m1', 'm1', 'm1', 'm2', 'm3', 'm4', 'm5', 'm6', 'm7', 'm8', 'm9', 'm9', 'm9', 'm9'];
  const r = evaluateWin(hand, [], { selfDraw: true });
  assert.ok(r.patterns.includes('清一色'));
  assert.ok(r.multiplier >= 8);
});

it('庄家 ×2、连庄再翻倍', () => {
  const hand = ['m1', 'm1', 'm1', 'm2', 'm3', 'm4', 's1', 's2', 's3', 'p5', 'p6', 'p7', 'z1', 'z1'];
  const r1 = evaluateWin(hand, [], { selfDraw: true, dealer: true, dealerStreak: 0 });
  const r2 = evaluateWin(hand, [], { selfDraw: true, dealer: true, dealerStreak: 1 });
  assert.ok(r2.multiplier >= r1.multiplier);
});

console.log('引擎');

function randPick(arr) {
  return arr[Math.floor(Math.random() * arr.length)];
}

/** 用随机策略把一局打完，验证状态机健壮性 */
function playRandom(allowRush, seed) {
  const game = new Game({ allowRush, rng: seededRng(seed) });
  let guard = 0;
  while (!game.over) {
    if (guard++ > 6000) throw new Error('对局未能在合理步数内结束');
    if (game.phase === PHASE.DRAW) {
      game.drawFor(game.turn);
      if (game.over) break;
    }
    if (game.phase === PHASE.DISCARD) {
      const seat = game.turn;
      const info = game.checkSelfWin(seat);
      if (info.win) { game.finish(seat, info, { type: 'self' }); break; }
      const gangs = game.selfGangOptions(seat);
      if (gangs.length && Math.random() < 0.5) {
        const g = randPick(gangs);
        game.doSelfGang(seat, g.tile, g.type);
        continue;
      }
      const p = game.player(seat);
      let r = game.discard(seat, randPick(p.hand));
      let tries = 0;
      while (r.error && tries++ < 30) r = game.discard(seat, randPick(p.hand));
      if (r.error) throw new Error('无法出牌: ' + r.error);
      continue;
    }
    if (game.phase === PHASE.CLAIM) {
      for (const cand of game.pending.candidates) {
        let decision = null;
        if (Math.random() < 0.6) {
          const a = randPick(cand.actions);
          decision = a === 'chi'
            ? { action: 'chi', tiles: randPick(cand.chiOptions) }
            : { action: a };
        }
        game.submitClaim(cand.seat, decision);
      }
      game.resolveClaims();
      continue;
    }
    throw new Error('未知阶段: ' + game.phase);
  }
  return game;
}

it('发牌：庄家 14 张，其余各 13 张', () => {
  const g = new Game({ rng: seededRng(7) });
  assert.equal(g.player(g.dealer).hand.length, 14);
  for (let s = 0; s < 4; s++) {
    if (s === g.dealer) continue;
    assert.equal(g.player(s).hand.length, 13);
  }
  assert.equal(g.player(nextSeat(g.dealer)).hand.length, 13);
  assert.ok(g.indicator);
});

it('随机策略可完整打完整局（自摸专属 20 局）', () => {
  for (let i = 0; i < 20; i++) {
    const g = playRandom(false, 1000 + i);
    assert.equal(g.over, true);
    assert.ok(g.result);
  }
});

it('随机策略可完整打完整局（允许捉冲 20 局）', () => {
  for (let i = 0; i < 20; i++) {
    const g = playRandom(true, 2000 + i);
    assert.equal(g.over, true);
    assert.ok(g.result);
  }
});

it('牌张守恒：手牌 + 亮牌 + 弃牌 + 牌墙 = 136', () => {
  const g = playRandom(true, 4321);
  const total = g.players.reduce(
    (n, p) => n + p.hand.length + p.melds.reduce((m, x) => m + x.tiles.length, 0) + p.discards.length,
    0,
  );
  assert.equal(total + g.wall.length + 1, 136); // +1 为财神翻牌
});

console.log('AI');

/** 全部由 AI 决策把一局打完 */
function playAI(allowRush, seed) {
  const game = new Game({ allowRush, rng: seededRng(seed), isAI: [true, true, true, true] });
  let guard = 0;
  while (!game.over) {
    if (guard++ > 6000) throw new Error('AI 对局未能在合理步数内结束');
    if (game.phase === PHASE.DRAW) {
      game.drawFor(game.turn);
      if (game.over) break;
    }
    if (game.phase === PHASE.DISCARD) {
      const seat = game.turn;
      const info = game.checkSelfWin(seat);
      if (info.win) { game.finish(seat, info, { type: 'self' }); break; }
      const gangs = game.selfGangOptions(seat);
      const g = chooseSelfGang(game, seat, gangs);
      if (g) { game.doSelfGang(seat, g.tile, g.type); continue; }
      const r = game.discard(seat, chooseDiscard(game, seat));
      if (r.error) throw new Error('AI 出牌被拒: ' + r.error);
      continue;
    }
    if (game.phase === PHASE.CLAIM) {
      for (const cand of game.pending.candidates) {
        game.submitClaim(cand.seat, chooseClaim(game, cand.seat, cand));
      }
      game.resolveClaims();
      continue;
    }
    throw new Error('未知阶段: ' + game.phase);
  }
  return game;
}

it('AI 不会主动打出财神（白板）', () => {
  const g = new Game({ rng: seededRng(3) });
  g.player(0).hand = ['m1', 'm2', 'm3', 'm4', 'm5', 'm6', 's1', 's2', 's3', 'p7', 'p8', 'z7', 'z1', 'z4'];
  g.turn = 0;
  g.phase = PHASE.DISCARD;
  const t = chooseDiscard(g, 0);
  assert.notEqual(t, 'z7');
  assert.ok(g.player(0).hand.includes(t));
});

it('AI 见到可胡的弃牌会选择胡', () => {
  const g = new Game({ rng: seededRng(5), allowRush: true });
  g.player(1).hand = ['m1', 'm2', 'm3', 'm4', 'm5', 'm6', 's1', 's2', 's3', 'p7', 'p8', 'z1', 'z1'];
  g.lastDiscard = { tile: 'p9', from: 0 };
  const d = chooseClaim(g, 1, { seat: 1, actions: ['win', 'peng'], chiOptions: [] });
  assert.equal(d.action, 'win');
});

it('结构评分：完整面子得分高于孤张', () => {
  const good = structureScore(['m1', 'm2', 'm3', 'm4', 'm5', 'm6']);
  const bad = structureScore(['m1', 'm5', 'z1', 'z4', 'p3', 's9']);
  assert.ok(good > bad);
});

it('AI 全自动对局可完整打完（各 15 局）', () => {
  for (let i = 0; i < 15; i++) assert.equal(playAI(false, 3000 + i).over, true);
  for (let i = 0; i < 15; i++) assert.equal(playAI(true, 4000 + i).over, true);
});

console.log(`\n${passed} 项通过`);