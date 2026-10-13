// 萧山麻将 —— 胡牌判定 + 番型计分
import {
  KIND_COUNT, isWild, toCounts, suitOf, idOfIndex,
} from './tiles.js';

/** 番型倍数表（依据公开的萧山麻将规则整理，可按牌桌习惯调整） */
export const PATTERNS = {
  自摸: 1,
  爆头: 2,
  碰碰胡: 2,
  全倒: 2,
  明杠开: 2,
  暗杠开: 4,
  财飘: 4,
  七对子: 4,      // 七对且用到财神
  清七对: 8,      // 七对且全部为天然牌
  清一色: 8,
  字一色: 16,
  天胡: 16,
  地胡: 16,
  豪华七对: 2,    // 每有一组 4 张相同牌，再 ×2
};

/** 单局封顶倍数（边锋规则：最大 64 倍基数） */
export const MAX_MULTIPLIER = 64;

const SUIT_OF_INDEX = (i) => Math.floor(i / 9); // 0-2 数牌, 3 字牌

/**
 * 通用胡牌搜索。
 * mode: 'any' 普通胡牌 | 'triplets' 只允许刻子（碰碰胡） | 'jokerPair' 将牌必须含财神（爆头）
 */
function search(counts, jokers, setsNeed, pairNeed, mode) {
  let i = 0;
  while (i < KIND_COUNT && counts[i] === 0) i++;

  if (i === KIND_COUNT) {
    const need = setsNeed * 3 + (pairNeed ? 2 : 0);
    return jokers === need;
  }

  // 将牌
  if (pairNeed) {
    if (mode === 'jokerPair') {
      if (counts[i] >= 1 && jokers >= 1) {
        counts[i]--;
        if (search(counts, jokers - 1, setsNeed, false, mode)) { counts[i]++; return true; }
        counts[i]++;
      }
      if (jokers >= 2 && counts[i] >= 0) {
        if (search(counts, jokers - 2, setsNeed, false, mode)) return true;
      }
    } else {
      if (counts[i] >= 2) {
        counts[i] -= 2;
        if (search(counts, jokers, setsNeed, false, mode)) { counts[i] += 2; return true; }
        counts[i] += 2;
      }
      if (counts[i] >= 1 && jokers >= 1) {
        counts[i]--;
        if (search(counts, jokers - 1, setsNeed, false, mode)) { counts[i]++; return true; }
        counts[i]++;
      }
      if (jokers >= 2) {
        if (search(counts, jokers - 2, setsNeed, false, mode)) return true;
      }
    }
  }

  if (setsNeed > 0) {
    // 刻子
    if (counts[i] >= 3) {
      counts[i] -= 3;
      if (search(counts, jokers, setsNeed - 1, pairNeed, mode)) { counts[i] += 3; return true; }
      counts[i] += 3;
    }
    if (counts[i] >= 2 && jokers >= 1) {
      counts[i] -= 2;
      if (search(counts, jokers - 1, setsNeed - 1, pairNeed, mode)) { counts[i] += 2; return true; }
      counts[i] += 2;
    }
    if (counts[i] >= 1 && jokers >= 2) {
      counts[i]--;
      if (search(counts, jokers - 2, setsNeed - 1, pairNeed, mode)) { counts[i]++; return true; }
      counts[i]++;
    }
    if (jokers >= 3) {
      if (search(counts, jokers - 3, setsNeed - 1, pairNeed, mode)) return true;
    }

    // 顺子（仅数牌）
    if (mode !== 'triplets') {
      const s = SUIT_OF_INDEX(i);
      const n = i % 9;
      if (s < 3 && n <= 6) {
        const has2 = counts[i + 1] > 0;
        const has3 = counts[i + 2] > 0;
        const miss = (has2 ? 0 : 1) + (has3 ? 0 : 1);
        if (jokers >= miss) {
          counts[i]--;
          if (has2) counts[i + 1]--;
          if (has3) counts[i + 2]--;
          const ok = search(counts, jokers - miss, setsNeed - 1, pairNeed, mode);
          counts[i]++;
          if (has2) counts[i + 1]++;
          if (has3) counts[i + 2]++;
          if (ok) return true;
        }
      }
    }
  }

  // 以最小牌为锚点，若它无法进入任何面子/将牌，则该分解不成立
  return false;
}

/** 基础胡牌判定（支持财神百搭） */
export function isWinningHand(handIds, meldCount = 0) {
  return decompose(handIds, meldCount, 'any') !== null;
}

/** 返回分解结果，用于番型判定；不成立返回 null */
export function decompose(handIds, meldCount, mode = 'any') {
  const setsNeed = 4 - meldCount;
  const needTiles = setsNeed * 3 + 2;
  if (handIds.length !== needTiles) return null;

  const jokers = handIds.filter(isWild).length;
  const counts = toCounts(handIds.filter((t) => !isWild(t)));
  const ok = search(counts, jokers, setsNeed, true, mode);
  return ok ? { setsNeed, jokers } : null;
}

/** 七对判定：无面子，14 张全为对子（财神可补单张） */
export function isSevenPairs(handIds) {
  if (handIds.length !== 14) return null;
  const jokers = handIds.filter(isWild).length;
  const counts = toCounts(handIds.filter((t) => !isWild(t)));
  let pairs = 0;
  let singles = 0;
  let quads = 0;
  for (const c of counts) {
    pairs += Math.floor(c / 2);
    singles += c % 2;
    if (c >= 4) quads++;
  }
  if (jokers < singles) return null;
  if ((jokers - singles) % 2 !== 0) return null;
  if (pairs + singles + (jokers - singles) / 2 !== 7) return null;
  return { jokers, usedJoker: jokers > 0, quads };
}

/** 列出能胡的牌种（听口） */
export function winningTiles(handIds, meldCount = 0) {
  const out = [];
  const setsNeed = 4 - meldCount;
  if (handIds.length !== setsNeed * 3 + 1) return out;
  for (let i = 0; i < KIND_COUNT; i++) {
    const id = idOfIndex(i);
    const test = [...handIds, id];
    if (setsNeed === 4 && test.length === 14 && isSevenPairs(test)) {
      out.push(id);
      continue;
    }
    if (isWinningHand(test, meldCount)) out.push(id);
  }
  return out;
}

/**
 * 计分。
 * @param {string[]} handIds 胡牌后的手牌（含刚摸到的那张）
 * @param {Array} melds 已亮出的面子 [{type:'chi'|'peng'|'gang'|'angang', tiles:[]}]
 * @param {object} ctx { selfDraw, dealer, dealerStreak, gangWin, playedWild, firstDraw, firstTurn }
 * @returns {{win:boolean, patterns:string[], multiplier:number, detail:object}}
 */
export function evaluateWin(handIds, melds = [], ctx = {}) {
  const meldCount = melds.length;
  const patterns = [];
  let mult = 1;

  const seven = meldCount === 0 ? isSevenPairs(handIds) : null;
  const normal = decompose(handIds, meldCount, 'any');

  if (!seven && !normal) return { win: false, patterns: [], multiplier: 0 };

  const add = (name) => {
    patterns.push(name);
    mult *= PATTERNS[name] ?? 1;
  };

  if (seven) {
    add(seven.usedJoker ? '七对子' : '清七对');
    for (let i = 0; i < seven.quads; i++) add('豪华七对');
  } else {
    if (decompose(handIds, meldCount, 'triplets')) add('碰碰胡');
    if (meldCount === 4) add('全倒');
    if (decompose(handIds, meldCount, 'jokerPair')) add('爆头');
  }

  // 自摸
  if (ctx.selfDraw) add('自摸');

  // 牌型检查：清一色 / 字一色
  const all = [...handIds, ...melds.flatMap((m) => m.tiles)];
  const suits = new Set(all.map(suitOf));
  if (suits.size === 1) {
    if (suits.has('z')) add('字一色');
    else add('清一色');
  }

  if (ctx.playedWild) add('财飘');
  if (ctx.gangWin === 'concealed') add('暗杠开');
  else if (ctx.gangWin === 'exposed') add('明杠开');

  if (ctx.firstDraw && ctx.dealer) add('天胡');
  if (ctx.firstTurn && !ctx.dealer) add('地胡');

  return {
    win: true,
    patterns,
    multiplier: Math.min(mult, MAX_MULTIPLIER),
    detail: { sevenPairs: !!seven },
  };
}
