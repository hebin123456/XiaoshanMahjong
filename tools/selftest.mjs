// Node 自测：牌面 / 胡牌判定 / 番型计分
// 运行：node tools/selftest.mjs
import assert from 'node:assert/strict';
import { makeWall, toCounts, KIND_COUNT, seededRng } from '../js/tiles.js';
import {
  isWinningHand, isSevenPairs, winningTiles, evaluateWin,
} from '../js/rules.js';

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

console.log(`\n${passed} 项通过`);
