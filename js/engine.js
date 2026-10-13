// 萧山麻将 —— 牌墙 / 发牌 / 回合状态机（无 UI 依赖，可 Node 自测）
import {
  makeWall, isWild, suitOf, numOf, kindIndex, toCounts, sortTiles,
  KIND_COUNT, idOfIndex,
} from './tiles.js';
import { isWinningHand, evaluateWin } from './rules.js';

export const PHASE = { DRAW: 'draw', DISCARD: 'discard', CLAIM: 'claim', OVER: 'over' };

/** 下家（右手边） */
export const nextSeat = (s) => (s + 1) % 4;
/** 上家（左手边） */
export const prevSeat = (s) => (s + 3) % 4;

/** 吃牌候选：返回可由手牌补齐的两张搭子 */
export function chiOptions(hand, tile) {
  const s = suitOf(tile);
  if (s === 'z') return [];
  const n = numOf(tile);
  const has = (num) => n > 0 && hand.some((t) => suitOf(t) === s && numOf(t) === num);
  const out = [];
  if (n - 2 >= 1 && has(n - 2) && has(n - 1)) out.push([s + (n - 2), s + (n - 1)]);
  if (n - 1 >= 1 && n + 1 <= 9 && has(n - 1) && has(n + 1)) out.push([s + (n - 1), s + (n + 1)]);
  if (n + 2 <= 9 && has(n + 1) && has(n + 2)) out.push([s + (n + 1), s + (n + 2)]);
  return out;
}

export class Game {
  constructor(opts = {}) {
    this.rng = opts.rng || Math.random;
    this.allowRush = !!opts.allowRush;      // 是否允许捉冲（点炮胡）
    this.base = opts.base ?? 1;              // 记分基数
    this.dealer = opts.dealer ?? 0;
    this.dealerStreak = opts.dealerStreak ?? 0;
    this.isAI = opts.isAI ?? [false, true, true, true];
    this.names = opts.names ?? ['你', '下家', '对家', '上家'];
    this.reset();
  }

  reset() {
    this.wall = makeWall(this.rng);
    this.players = [0, 1, 2, 3].map((s) => ({
      seat: s,
      isAI: this.isAI[s],
      name: this.names[s],
      hand: [],
      melds: [],
      discards: [],
      claimFrom: {},        // 承包统计：{来源座位: 吃碰杠摊数}
      chiJustMade: null,    // 刚吃出的牌，用于“吃张打张”限制
      chiCount: 0,
      playedWild: false,    // 本局是否打出过财神
    }));

    for (let i = 0; i < 13; i++) {
      for (let s = 0; s < 4; s++) this.players[s].hand.push(this.wall.shift());
    }
    this.players[this.dealer].hand.push(this.wall.shift()); // 庄家 14 张

    // 财神翻牌：牌墙剩余的末张
    this.indicator = this.wall.pop();
    for (const p of this.players) p.hand = sortTiles(p.hand);

    this.turn = this.dealer;
    this.phase = PHASE.DISCARD;   // 庄家起手即有 14 张，先出牌
    this.turnCount = [0, 0, 0, 0];
    this.gangWinType = null;      // 'exposed' | 'concealed'
    this.lastDraw = null;
    this.lastDrawIsFirst = false;
    this.lastDiscard = null;
    this.pending = null;
    this.dealerFirstHand = true;  // 天胡判定
    this.result = null;
    this.log = [];
  }

  get over() {
    return this.phase === PHASE.OVER;
  }

  player(seat) {
    return this.players[seat];
  }

  addLog(text) {
    this.log.push(text);
    if (this.log.length > 200) this.log.shift();
  }

  // ---------- 摸牌 / 打牌 ----------

  /** 当前玩家摸牌；返回摸到的牌 */
  drawFor(seat) {
    if (this.wall.length === 0) return this.drawnOut();
    const tile = this.wall.shift();
    const p = this.players[seat];
    this.lastDrawIsFirst = this.turnCount[seat] === 0;
    p.hand.push(tile);
    p.hand = sortTiles(p.hand);
    this.turnCount[seat]++;
    this.turn = seat;
    this.lastDraw = { seat, tile };
    this.lastDiscard = null;
    this.gangWinType = null;
    this.phase = PHASE.DISCARD;
    return tile;
  }

  /** 杠后补牌 */
  drawReplacement(seat, gangType) {
    if (this.wall.length === 0) return this.drawnOut();
    const tile = this.wall.shift();
    const p = this.players[seat];
    p.hand.push(tile);
    p.hand = sortTiles(p.hand);
    this.turn = seat;
    this.lastDraw = { seat, tile };
    this.lastDrawIsFirst = false;
    this.gangWinType = gangType;
    this.phase = PHASE.DISCARD;
    return tile;
  }

  /** 出牌；返回可申领信息 */
  discard(seat, tileId) {
    const p = this.players[seat];
    const blocked = this.discardBlocked(seat, tileId);
    if (blocked) return { error: blocked };
    const idx = p.hand.lastIndexOf(tileId);
    if (idx < 0) throw new Error('手牌中没有这张牌: ' + tileId);
    p.hand.splice(idx, 1);
    p.discards.push(tileId);
    if (isWild(tileId)) p.playedWild = true;
    p.chiJustMade = null;
    if (seat === this.dealer) this.dealerFirstHand = false;
    this.gangWinType = null;
    this.lastDiscard = { tile: tileId, from: seat, claimed: false };

    const candidates = this.computeClaims(tileId, seat);
    if (candidates.length === 0) {
      this.advanceTurn(seat);
      return { candidates };
    }
    this.pending = {
      tile: tileId, from: seat, candidates, decisions: {}, order: candidates.map((c) => c.seat),
    };
    this.phase = PHASE.CLAIM;
    return { candidates };
  }

  advanceTurn(fromSeat) {
    this.turn = nextSeat(fromSeat);
    this.phase = PHASE.DRAW;
    this.pending = null;
  }

  /**
   * “吃张打张”限制：已吃了上家 2 摊的玩家，刚吃成的顺子不能再打掉其相邻牌
   * （如吃出 567 条后不能打 4 条或 8 条）。
   */
  discardBlocked(seat, tileId) {
    const p = this.players[seat];
    if (!p.chiJustMade || p.chiCount < 2) return null;
    const setSuits = new Set(p.chiJustMade.map(suitOf));
    if (setSuits.size !== 1 || setSuits.has('z')) return null;
    if (!setSuits.has(suitOf(tileId))) return null;
    const nums = p.chiJustMade.map(numOf);
    const lo = Math.min(...nums);
    const hi = Math.max(...nums);
    const n = numOf(tileId);
    if (n === lo - 1 || n === hi + 1) {
      return `吃了上家 2 摊后不能吃张打张（不能打 ${tileId}）`;
    }
    return null;
  }

  /** 弃牌后的可申领动作 */
  computeClaims(tile, fromSeat) {
    if (isWild(tile)) return []; // 打财神，其他三家不能吃碰
    const cands = [];
    for (let s = 0; s < 4; s++) {
      if (s === fromSeat) continue;
      const p = this.players[s];
      const n = p.hand.filter((t) => t === tile).length;
      const actions = [];
      if (this.allowRush && isWinningHand([...p.hand, tile], p.melds.length)) actions.push('win');
      if (n >= 2) actions.push('peng');
      if (n >= 3) actions.push('gang');
      if (s === nextSeat(fromSeat)) {
        const opts = chiOptions(p.hand, tile);
        if (opts.length) actions.push('chi');
      }
      if (actions.length) {
        cands.push({
          seat: s,
          actions,
          chiOptions: actions.includes('chi') ? chiOptions(p.hand, tile) : [],
        });
      }
    }
    return cands;
  }

  /** 记录一个玩家对弃牌的申报决策；decision 为 null 表示过 */
  submitClaim(seat, decision) {
    if (this.phase !== PHASE.CLAIM || !this.pending) return;
    this.pending.decisions[seat] = decision || null;
  }

  get claimsResolved() {
    if (!this.pending) return true;
    return this.pending.order.every((s) => s in this.pending.decisions);
  }

  /** 结算所有申报：胡 > 杠 > 碰 > 吃；同级按离出牌者最近的顺序 */
  resolveClaims() {
    const pd = this.pending;
    if (!pd) return null;
    const rank = { win: 4, gang: 3, peng: 2, chi: 1 };
    let best = null;
    for (const cand of pd.candidates) {
      const d = pd.decisions[cand.seat];
      if (!d || !d.action) continue;
      const dist = (cand.seat - pd.from + 4) % 4;
      const score = rank[d.action] * 10 - dist;
      if (!best || score > best.score) best = { ...d, seat: cand.seat, score };
    }
    this.pending = null;
    if (!best) {
      const from = pd.from;
      this.advanceTurn(from);
      return { type: 'pass', from };
    }
    return this.applyClaim(best, pd);
  }

  applyClaim(best, pd) {
    const p = this.players[best.seat];
    const tile = pd.tile;
    this.turn = best.seat;
    this.lastDiscard = { tile, from: pd.from, claimed: true };

    if (best.action === 'win') {
      p.hand.push(tile);
      p.hand = sortTiles(p.hand);
      this.players[pd.from].discards.pop();
      const info = evaluateWin(p.hand, p.melds, {
        selfDraw: false,
        dealer: best.seat === this.dealer,
        dealerStreak: this.dealerStreak,
        playedWild: p.playedWild,
        gangWin: null,
      });
      this.finish(best.seat, info, { type: 'rush', from: pd.from });
      return { type: 'win', seat: best.seat, from: pd.from };
    }

    const removeFrom = (count, kind) => {
      for (let i = 0; i < count;) {
        const k = p.hand.lastIndexOf(kind);
        if (k < 0) break;
        p.hand.splice(k, 1);
        i++;
      }
      p.hand = sortTiles(p.hand);
    };

    if (best.action === 'chi') {
      const pair = best.tiles;
      removeFrom(1, pair[0]);
      removeFrom(1, pair[1]);
      p.melds.push({ type: 'chi', tiles: sortTiles([...pair, tile]), from: pd.from });
      this.bumpClaim(p, pd.from);
      p.chiCount++;
      p.chiJustMade = sortTiles([...pair, tile]);
      this.players[pd.from].discards.pop();
      this.lastDraw = null;
      this.phase = PHASE.DISCARD;
      return { type: 'chi', seat: best.seat, from: pd.from, tiles: [...pair, tile] };
    }

    if (best.action === 'peng') {
      removeFrom(2, tile);
      p.melds.push({ type: 'peng', tiles: [tile, tile, tile], from: pd.from });
      this.bumpClaim(p, pd.from);
      this.players[pd.from].discards.pop();
      this.lastDraw = null;
      this.phase = PHASE.DISCARD;
      return { type: 'peng', seat: best.seat, from: pd.from };
    }

    if (best.action === 'gang') {
      removeFrom(3, tile);
      p.melds.push({ type: 'gang', tiles: [tile, tile, tile, tile], from: pd.from });
      this.bumpClaim(p, pd.from);
      this.players[pd.from].discards.pop();
      const drawn = this.drawReplacement(best.seat, 'exposed');
      return { type: 'gang', seat: best.seat, from: pd.from, tile, drawn };
    }
    return null;
  }

  bumpClaim(p, fromSeat) {
    p.claimFrom[fromSeat] = (p.claimFrom[fromSeat] || 0) + 1;
  }

  // ---------- 自摸杠 ----------

  /** 自己回合可执行的杠：暗杠 / 补杠 */
  selfGangOptions(seat) {
    const p = this.players[seat];
    if (p.hand.length % 3 !== 2) return []; // 刚摸牌后应为 3n+2
    const opts = [];
    const counts = toCounts(p.hand.filter((t) => !isWild(t)));
    for (let i = 0; i < KIND_COUNT; i++) {
      if (counts[i] === 4) opts.push({ type: 'angang', tile: idOfIndex(i) });
    }
    for (const m of p.melds) {
      if (m.type === 'peng' && p.hand.includes(m.tiles[0])) {
        opts.push({ type: 'bugang', tile: m.tiles[0] });
      }
    }
    return opts;
  }

  /** 执行暗杠 / 补杠，返回补到的牌 */
  doSelfGang(seat, tile, type) {
    const p = this.players[seat];
    if (type === 'angang') {
      for (let i = 0; i < 4; i++) p.hand.splice(p.hand.lastIndexOf(tile), 1);
      p.hand = sortTiles(p.hand);
      p.melds.push({ type: 'angang', tiles: [tile, tile, tile, tile], from: null });
      this.addLog(`${p.name} 暗杠 ${tile}`);
      return this.drawReplacement(seat, 'concealed');
    }
    // 补杠
    p.hand.splice(p.hand.lastIndexOf(tile), 1);
    const m = p.melds.find((x) => x.type === 'peng' && x.tiles[0] === tile);
    if (m) { m.type = 'gang'; m.tiles = [tile, tile, tile, tile]; }
    this.addLog(`${p.name} 补杠 ${tile}`);
    return this.drawReplacement(seat, 'exposed');
  }

  // ---------- 胡牌判定 ----------

  /** 当前玩家自摸判定（必须是刚摸到牌，或庄家起手 14 张） */
  checkSelfWin(seat) {
    const p = this.players[seat];
    const justDrew = !!(this.lastDraw && this.lastDraw.seat === seat);
    const initialHand = seat === this.dealer && this.dealerFirstHand && p.melds.length === 0;
    if (!justDrew && !initialHand) {
      return { win: false, patterns: [], multiplier: 0 };
    }
    const isDealer = seat === this.dealer;
    return evaluateWin(p.hand, p.melds, {
      selfDraw: true,
      dealer: isDealer,
      dealerStreak: this.dealerStreak,
      gangWin: this.gangWinType,
      playedWild: p.playedWild,
      firstDraw: isDealer && this.dealerFirstHand && p.melds.length === 0,
      firstTurn: !isDealer && this.lastDrawIsFirst && p.melds.length === 0,
    });
  }

  finish(winnerSeat, info, extra = {}) {
    this.phase = PHASE.OVER;
    const from = extra.from ?? null;
    const dealerMult = winnerSeat === this.dealer
      ? Math.min(2 ** (1 + this.dealerStreak), 8)
      : 1;
    const mult = Math.min(info.multiplier * dealerMult, 64);
    const settlement = this.settle(winnerSeat, mult, { type: extra.type, from });
    this.result = {
      winner: winnerSeat,
      dealer: winnerSeat === this.dealer,
      type: extra.type || 'self',
      from,
      patterns: info.patterns,
      multiplier: mult,
      ...settlement,
    };
    this.addLog(`${this.players[winnerSeat].name} 胡牌：${info.patterns.join('、')}，${mult} 倍`);
    return this.result;
  }

  /** 结算：自摸三家付；捉冲点炮者独付；触发承包则承包者独付并加倍 */
  settle(winnerSeat, mult, info) {
    const base = this.base * mult;
    const scores = [0, 0, 0, 0];
    const winner = this.players[winnerSeat];

    // 承包：胡牌者从同一位来源吃碰杠达 3 摊
    let baoSeat = null;
    for (const [k, v] of Object.entries(winner.claimFrom)) {
      if (v >= 3) { baoSeat = Number(k); break; }
    }
    if (info.type === 'rush' && info.from !== null) {
      const vs = winner.claimFrom[info.from] || 0;
      if (vs >= 3) baoSeat = info.from;
    }

    if (baoSeat !== null && baoSeat !== winnerSeat) {
      const amount = base * 3 * 2;
      scores[baoSeat] -= amount;
      scores[winnerSeat] += amount;
      return { scores, baoSeat, note: '承包加倍' };
    }

    if (info.type === 'rush' && info.from !== null) {
      scores[info.from] -= base * 3;
      scores[winnerSeat] += base * 3;
    } else {
      for (let s = 0; s < 4; s++) {
        if (s === winnerSeat) continue;
        scores[s] -= base;
        scores[winnerSeat] += base;
      }
    }
    return { scores, baoSeat: null };
  }

  drawnOut() {
    this.phase = PHASE.OVER;
    this.result = { winner: null, type: 'draw', scores: [0, 0, 0, 0] };
    this.addLog('牌墙摸完，本局流局，庄家连庄');
    return null;
  }
}
