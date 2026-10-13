// 萧山麻将 —— 电脑 AI 策略
// 核心是一个“手牌结构评估”启发式：把牌拆成面子（刻子/顺子）与搭子（对子/两面/嵌张），
// 财神（白板）视为万能牌参与拆解。出牌时选择“打掉后结构最好”的那张。
import { KIND_COUNT, isWild, toCounts } from './tiles.js';

/**
 * 递归拆解手牌，求最大面子/搭子组合。
 * @returns {{sets:number, partials:number}}
 */
function bestForm(counts, jokers, setsLeft, pairLeft, memo, depthKey) {
  let i = 0;
  while (i < KIND_COUNT && counts[i] === 0) i++;

  const key = depthKey + '|' + setsLeft + '|' + (pairLeft ? 1 : 0) + '|' + jokers + '|' + counts.join('');
  const cached = memo.get(key);
  if (cached) return cached;

  if (i === KIND_COUNT) {
    // 只剩财神：3 张凑面子，2 张凑搭子
    let sets = 0;
    let partials = 0;
    let j = jokers;
    while (setsLeft - sets > 0 && j >= 3) { sets++; j -= 3; }
    if (pairLeft && j >= 2) { partials++; j -= 2; }
    while (sets + partials < 4 && j >= 2) { partials++; j -= 2; }
    const res = { sets, partials };
    memo.set(key, res);
    return res;
  }

  let best = { sets: 0, partials: 0 };
  const consider = (r, s, p) => {
    const sets = s + r.sets;
    const partials = p + r.partials;
    if (sets + partials > 4) return;
    if (sets * 2 + partials > best.sets * 2 + best.partials) best = { sets, partials };
  };

  const isSuited = i < 27;
  const n = i % 9;

  if (setsLeft > 0) {
    if (counts[i] >= 3) {
      counts[i] -= 3;
      consider(bestForm(counts, jokers, setsLeft - 1, pairLeft, memo, depthKey), 1, 0);
      counts[i] += 3;
    }
    if (counts[i] >= 2 && jokers >= 1) {
      counts[i] -= 2;
      consider(bestForm(counts, jokers - 1, setsLeft - 1, pairLeft, memo, depthKey), 1, 0);
      counts[i] += 2;
    }
    if (counts[i] >= 1 && jokers >= 2) {
      counts[i] -= 1;
      consider(bestForm(counts, jokers - 2, setsLeft - 1, pairLeft, memo, depthKey), 1, 0);
      counts[i] += 1;
    }
    if (jokers >= 3) {
      consider(bestForm(counts, jokers - 3, setsLeft - 1, pairLeft, memo, depthKey), 1, 0);
    }
    if (isSuited && n <= 6) {
      const has2 = counts[i + 1] > 0;
      const has3 = counts[i + 2] > 0;
      const miss = (has2 ? 0 : 1) + (has3 ? 0 : 1);
      if (jokers >= miss) {
        counts[i]--;
        if (has2) counts[i + 1]--;
        if (has3) counts[i + 2]--;
        consider(bestForm(counts, jokers - miss, setsLeft - 1, pairLeft, memo, depthKey), 1, 0);
        counts[i]++;
        if (has2) counts[i + 1]++;
        if (has3) counts[i + 2]++;
      }
    }
  }

  // 搭子：对子 / 两面 / 嵌张
  if (pairLeft && counts[i] >= 2) {
    counts[i] -= 2;
    consider(bestForm(counts, jokers, setsLeft, false, memo, depthKey), 0, 1);
    counts[i] += 2;
  }
  if (pairLeft && counts[i] >= 1 && jokers >= 1) {
    counts[i] -= 1;
    consider(bestForm(counts, jokers - 1, setsLeft, false, memo, depthKey), 0, 1);
    counts[i] += 1;
  }
  if (counts[i] >= 2) {
    counts[i] -= 2;
    consider(bestForm(counts, jokers, setsLeft, pairLeft, memo, depthKey), 0, 1);
    counts[i] += 2;
  }
  if (counts[i] >= 1 && jokers >= 1) {
    counts[i] -= 1;
    consider(bestForm(counts, jokers - 1, setsLeft, pairLeft, memo, depthKey), 0, 1);
    counts[i] += 1;
  }
  if (isSuited) {
    if (n + 1 <= 9 && counts[i + 1] > 0) {
      counts[i]--; counts[i + 1]--;
      consider(bestForm(counts, jokers, setsLeft, pairLeft, memo, depthKey), 0, 1);
      counts[i]++; counts[i + 1]++;
    }
    if (n + 1 <= 9 && jokers >= 1) {
      counts[i]--;
      consider(bestForm(counts, jokers - 1, setsLeft, pairLeft, memo, depthKey), 0, 1);
      counts[i]++;
    }
    if (n + 2 <= 9 && counts[i + 2] > 0) {
      counts[i]--; counts[i + 2]--;
      consider(bestForm(counts, jokers, setsLeft, pairLeft, memo, depthKey), 0, 1);
      counts[i]++; counts[i + 2]++;
    }
    if (n + 2 <= 9 && jokers >= 1) {
      counts[i]--;
      consider(bestForm(counts, jokers - 1, setsLeft, pairLeft, memo, depthKey), 0, 1);
      counts[i]++;
    }
  }

  // 孤立牌：直接把锚点当作无用牌
  counts[i]--;
  consider(bestForm(counts, jokers, setsLeft, pairLeft, memo, depthKey), 0, 0);
  counts[i]++;

  memo.set(key, best);
  return best;
}

/** 手牌结构评分：面子权重 2，搭子权重 1 */
export function structureScore(tiles, meldCount = 0) {
  const jokers = tiles.filter(isWild).length;
  const counts = toCounts(tiles.filter((t) => !isWild(t)));
  const memo = new Map();
  const r = bestForm(counts, jokers, 4 - meldCount, true, memo, '');
  return r.sets * 2 + r.partials;
}

/** 计算候选出牌的得分（越高越该打） */
function rankDiscards(game, seat) {
  const p = game.player(seat);
  const melds = p.melds.length;
  const seen = new Set();
  const out = [];
  for (const t of p.hand) {
    if (seen.has(t)) continue;
    seen.add(t);
    if (game.discardBlocked(seat, t)) continue;
    const idx = p.hand.lastIndexOf(t);
    const rest = p.hand.slice(0, idx).concat(p.hand.slice(idx + 1));
    // 打掉 t 之后剩余结构的得分越高，说明 t 越没用
    out.push({ tile: t, keep: structureScore(rest, melds) });
  }
  out.sort((a, b) => {
    if (b.keep !== a.keep) return b.keep - a.keep;
    // 同分时优先打孤张字牌 / 幺九
    const safe = (t) => (t[0] === 'z' ? 0 : (t[1] === '1' || t[1] === '9' ? 1 : 2));
    return safe(a.tile) - safe(b.tile);
  });
  return out;
}

/** 选择出牌 */
export function chooseDiscard(game, seat) {
  const ranked = rankDiscards(game, seat);
  if (!ranked.length) {
    // 极端情况：只能打财神
    const p = game.player(seat);
    return p.hand[0];
  }
  return ranked[0].tile;
}

/** 对弃牌的申报决策；candidate 来自 engine.computeClaims */
export function chooseClaim(game, seat, candidate) {
  const p = game.player(seat);
  if (candidate.actions.includes('win')) return { action: 'win' };

  const melds = p.melds.length;
  const before = structureScore(p.hand, melds);
  const tile = game.lastDiscard.tile;

  if (candidate.actions.includes('gang')) {
    const rest = removeCopies(p.hand, tile, 3);
    const after = 2 + structureScore(rest, melds + 1) + 1; // 杠后还能补一张
    if (after >= before) return { action: 'gang' };
  }
  if (candidate.actions.includes('peng')) {
    const rest = removeCopies(p.hand, tile, 2);
    const after = 2 + structureScore(rest, melds + 1);
    if (after > before) return { action: 'peng' };
  }
  if (candidate.actions.includes('chi') && candidate.chiOptions.length) {
    let bestOpt = null;
    let bestScore = before;
    for (const opt of candidate.chiOptions) {
      let rest = removeCopies(p.hand, opt[0], 1);
      rest = removeCopies(rest, opt[1], 1);
      const after = 2 + structureScore(rest, melds + 1);
      // 吃牌会亮牌，要求明显收益
      if (after >= before + 2 && after > bestScore) { bestScore = after; bestOpt = opt; }
    }
    if (bestOpt) return { action: 'chi', tiles: bestOpt };
  }
  return null;
}

/** 自己的暗杠 / 补杠决策 */
export function chooseSelfGang(game, seat, options) {
  if (!options.length) return null;
  const p = game.player(seat);
  const melds = p.melds.length;
  const before = structureScore(p.hand, melds);
  let best = null;
  let bestScore = -Infinity;
  for (const opt of options) {
    if (opt.type === 'angang') {
      const rest = removeCopies(p.hand, opt.tile, 4);
      const after = 2 + structureScore(rest, melds + 1) + 1;
      if (after >= before && after > bestScore) { bestScore = after; best = opt; }
    } else {
      // 补杠：把已碰的刻子升级，几乎总是划算
      const rest = removeCopies(p.hand, opt.tile, 1);
      const after = 2 + structureScore(rest, melds) + 1;
      if (after > bestScore) { bestScore = after; best = opt; }
    }
  }
  return best;
}

function removeCopies(tiles, tile, count) {
  const out = [...tiles];
  for (let i = 0; i < count; i++) {
    const k = out.lastIndexOf(tile);
    if (k >= 0) out.splice(k, 1);
  }
  return out;
}
