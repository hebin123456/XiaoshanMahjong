// 萧山麻将 —— 牌面定义与工具
// 花色：m=万, s=条, p=筒, z=字（1东 2南 3西 4北 5中 6发 7白）
// 全副 136 张（去掉 8 张花），每种 4 张。
// 财神（百搭）= 白板 z7

export const SUITS = ['m', 's', 'p', 'z'];
export const SUIT_NAMES = { m: '万', s: '条', p: '筒', z: '字' };
export const ZI_NAMES = ['东', '南', '西', '北', '中', '发', '白'];
export const NUM_CN = ['', '一', '二', '三', '四', '五', '六', '七', '八', '九'];

/** 财神牌：白板 */
export const WILD_ID = 'z7';

const KINDS = [];
for (const s of ['m', 's', 'p']) {
  for (let n = 1; n <= 9; n++) KINDS.push(s + n);
}
for (let n = 1; n <= 7; n++) KINDS.push('z' + n);

export const TILE_KINDS = KINDS; // 34 种
export const KIND_COUNT = KINDS.length;

const INDEX_OF = new Map(KINDS.map((k, i) => [k, i]));

/** 牌种 -> 0..33 的序号 */
export function kindIndex(id) {
  const i = INDEX_OF.get(id);
  if (i === undefined) throw new Error('未知牌种: ' + id);
  return i;
}

/** 序号 -> 牌种 */
export function idOfIndex(i) {
  return KINDS[i];
}

export function suitOf(id) {
  return id[0];
}

export function numOf(id) {
  return Number(id.slice(1));
}

export function isHonor(id) {
  return id[0] === 'z';
}

export function isWild(id) {
  return id === WILD_ID;
}

/** 中文显示名，如 m1 -> 一万, z5 -> 红中, z7 -> 白板 */
export function cnName(id) {
  const s = id[0];
  const n = numOf(id);
  if (s === 'z') {
    if (n === 5) return '红中';
    if (n === 6) return '发财';
    if (n === 7) return '白板';
    return ZI_NAMES[n - 1];
  }
  return NUM_CN[n] + SUIT_NAMES[s];
}

/** 短标签，用于牌面绘制（如 1万 / 5条 / 中） */
export function shortLabel(id) {
  const s = id[0];
  const n = numOf(id);
  if (s === 'z') {
    return ['东', '南', '西', '北', '中', '发', '白'][n - 1];
  }
  return n + SUIT_NAMES[s];
}

/** 生成一副洗好的牌（136 张，字符串 id 数组） */
export function makeWall(rng = Math.random) {
  const wall = [];
  for (const k of KINDS) {
    for (let c = 0; c < 4; c++) wall.push(k);
  }
  for (let i = wall.length - 1; i > 0; i--) {
    const j = Math.floor(rng() * (i + 1));
    [wall[i], wall[j]] = [wall[j], wall[i]];
  }
  return wall;
}

/** 可复现随机数（xorshift32），用于自测 */
export function seededRng(seed = 20261013) {
  let x = seed >>> 0;
  if (x === 0) x = 0x9e3779b9;
  return function () {
    x ^= x << 13;
    x >>>= 0;
    x ^= x >>> 17;
    x ^= x << 5;
    x >>>= 0;
    return x / 0x100000000;
  };
}

/** 把牌数组转成 34 长度计数数组 */
export function toCounts(ids) {
  const counts = new Array(KIND_COUNT).fill(0);
  for (const id of ids) counts[kindIndex(id)]++;
  return counts;
}

/** 排序：按牌种序号 */
export function sortTiles(ids) {
  return [...ids].sort((a, b) => kindIndex(a) - kindIndex(b));
}
