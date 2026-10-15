# 萧山麻将 · XiaoshanMahjong

**安卓原生 App**：单机版萧山麻将，**你 vs 3 个电脑 AI**，横屏对局；适配折叠屏与超宽屏
（展开/折叠、分屏实时重排，无黑边）。

技术路线：基于开源麻将工程 **cocostudio-mahjong**（C++ / Cocos2d-x 3.17）改造，
在其成熟的麻将桌 UI、牌型算法与电脑 AI 之上，把规则层改造成**萧山麻将**。
> 说明：本项目已**放弃早期的 Kotlin / Jetpack Compose 实现**，仓库从零重来，现以
> Cocos2d-x 原生工程为唯一主线。

---

## 进度（Progress）

### 已完成 ✅

- [x] **并入上游源码**：`native/` 下完整收录 cocostudio-mahjong（Classes / Resources / cocos2d / cocosstudio / proj.android）。
- [x] **补全构建依赖**：上游缺失 `external/android/cpufeatures`，已从 NDK 21.4.7075529 拷入 `native/cocos2d/external/android/cpufeatures`。
- [x] **本地环境就绪**：JDK 17、Android SDK（platform 35 / build-tools 35）、NDK 21.4.7075529。
- [x] **构建配置现代化**（`native/proj.android/`）：
  - Gradle wrapper → **8.11.1**；AGP → **8.7.3**；
  - 仓库由 `jcenter()` 改为 `google()` + `mavenCentral()`；
  - `compileSdk / targetSdk = 35`，`minSdk = 21`；
  - ABI 暂保持 `armeabi-v7a`（上游 cocos2d 外部库仅有 arm32 预编译，arm64 待补齐）；
  - 移除 `buildToolsVersion` / `dexOptions` / 旧 `lintOptions`，改用 `namespace` 与 `lint {}`；
  - AndroidManifest 去掉 `package`，补 `android:exported`（适配 Android 12+）。

### 进行中 🚧

- [x] 补齐 `arm64-v8a` 外部预编译库（png/jpeg/zlib/freetype），放开 64 位打包。
- [x] **适配 Android 16KB 页大小**：`APP_LDFLAGS` 增加 `-Wl,-z,max-page-size=16384`，满足 Android 15+ 对 16KB 内存页机型的要求。
- [x] CI 出包验证（双架构 APK 可安装于 64 位机型）。
- [ ] 萧山规则适配（C++ 逻辑层，见下）。
  - [x] **财神万能**：翻出的真财神（`s_JokerCard`）可替代任意牌组成顺/刻/将（`GameLogic::analyseCardJoker`）；白板（z7 / 0x37）为其替身，只当财神那张原牌用。
  - [x] 翻财神：发牌后翻出的那张牌即本局真财神，本身即为万能牌；白板为其替身（只当财神那张牌用）。
  - [x] 财神打出限制（打出白板时其余三家不能吃/碰/杠）。
  - [x] 财神不能被放冲：打出财神（即「飘」）时，其余三家不能点炮胡该牌，只能自摸。
  - [x] 规则开关框架：`GameLogic::RuleFlags`（允许捉冲 / 有财神必须爆头 / 允许吃三摊）+ `GameConfig` 持久化。
    - [x] 允许捉冲（点炮胡）：关闭后非自摸且非抢杠不可胡。
    - [x] 有财神必须爆头：开启后手中财神未作将（七对除外）不可胡。
  - [x] 允许吃三摊（承包）：未开启时每家最多 2 个明牌组合（不能碰/杠第 3 摊）；开启时已亮三摊者点炮胡，放炮者独付并加倍。
  - [x] 抢庄制：谁胡谁做庄（一炮多响取离放炮者最近者），流局连庄。
  - [x] 庄家加倍：庄家自摸、或庄家放炮，结算番数 ×2（封顶 64）。
  - [-] 吃张打张：萧山麻将无「吃」，且引擎未实现吃牌（仅碰/杠），该规则不适用。
  - [ ] 番型倍数表（爆头/碰碰胡/七对/清一色/天胡地胡…）。
    - [x] 清一色判定忽略财神花色（含财神的清一色仍算清一色）。
    - [x] 爆头（财神作将）识别 CHR_BT。
    - [x] 字一色识别 CHR_ZY（全字牌，财神白板本身即字牌）。
    - [x] `getHuFanShu` 萧山倍数表：大对子（碰碰胡）×4 / 七对×4 / 豪华七对×8 / 清七对×8 / 清一色×8 / 字一色×16 /
      天胡·地胡×16，爆头×2、杠开×2 叠加，封顶 64（已接入引擎结算）。
    - [x] 明杠开 / 暗杠开区分：`GameEngine` 记录最近一次杠的明/暗（`m_bLastGangPublic`），
      `getHuFanShu` 按明杠开×2 计，暗杠开在结算时再 ×2（合计 ×4）。
    - [x] 财飘×4：本轮打出过财神（白板）且最终仍以财神作将（爆头）时，结算再 ×4。
    - [x] 十风子保底：胡牌者累计打出 10 颗字牌（东南西北中发白，真财神也算）保底基本胡×1，11 颗保底爆头×2（`GameEngine::getDiscardFengZiCount`，自摸/捉炮结算均已接入）。
    - [x] 全风子免组合：手牌与组合全为字牌（真财神可充字牌）时免拆顺/刻直接认胡，按字一色×16；并豁免「有财神必须爆头」。
    - [x] 豪华七对：七对中含四张相同的牌，×8（`CHR_HQD`）。
    - [x] 连庄倍数：庄×2 / 二连庄×4 / 三连庄×8……（2^连庄数）；流局连庄数+1，庄家自己胡牌连庄+1，换庄重置为1（`GameEngine::m_cbBankerStreak`，庄家头像有「庄/N连庄」角标）。
    - [x] 番数封顶可配置：设置中可选 16/32/64，默认 32（`GameConfig::m_iFanCap`，`getHuFanShu` 与结算加倍全部走封顶）。
    - [x] 修复：`m_cbDiscardCount` 未按局清零，跨局累加会越界写 `m_cbDiscardCard`；已在 `init()` 中清零。
    - [x] 修正：天胡/地胡存于 `huSpecial`，`getHuFanShu` 改读 `huSpecial`（原读 `huRight` 永不生效）。

---

## 萧山麻将规则要点（待适配到 C++ 逻辑层）

| 规则 | 说明 | 目标位置 |
| --- | --- | --- |
| 财神（万能） | **发牌后翻出的那张牌 = 财神**，是万能牌；**白板是财神的替身**（只当财神那张原牌用，不是万能牌） | `GameLogic` 胡牌/听牌分析 |
| 财神打出限制 | 打出财神（白板）时，其余三家不能吃 / 碰 / 杠 | `GameEngine` 动作裁决 |
| 财神不能被放冲 | 打出财神（即「飘」）时，该牌不能被点炮胡；放冲只针对普通牌 | `GameEngine` 动作裁决 |
| 十风子 | 累计打出 10 颗字牌（东南西北中发白，含真财神）→ 保底基本胡；11 颗 → 保底爆头 | `GameEngine` 结算 |
| 全风子 | 手牌全为字牌时免组合即认胡，按字一色 ×16 | `GameLogic` 胡牌判定 |
| 有财神必须爆头 | 开关项：开启时手中财神至少留 1 张做将牌（爆头 ×2） | 规则开关 + 番型 |
| 允许捉冲（点炮胡） | 开关项：关闭即「自摸专属」，只能自摸 | `GameLogic::analyseHuCard` |
| 允许吃三摊（承包） | 开关项：关闭时每家最多吃/碰/杠 2 摊；开启时第 3 摊触发**承包**绑定，绑定方独付并加倍 | `GameEngine` 结算 |
| 抢庄制 | 谁胡谁做庄，流局连庄 | `GameEngine` 局末 |
| 吃张打张 | 已吃上家 2 摊后不允许吃张打张 | `GameEngine` 出牌校验 |

### 番型与倍数（目标）

自摸 ×1（基础）｜爆头 ×2｜大对子（碰碰胡）×4｜明杠开 ×2｜暗杠开 ×4｜
财飘 ×4｜七对 ×4｜豪华七对 ×8｜清七对 ×8｜清一色 ×8｜字一色 ×16｜天胡 ×16｜地胡 ×16。
倍数可叠加；连庄倍数庄×2 / 二连庄×4 / 三连庄×8……（流局连庄+1）。
番数封顶可在主界面设置中配置（16/32/64，默认 32 倍）。

### 设置分层

- **主界面设置**：完整规则配置（AI 强度、三个规则开关、起始积分、基本分、番数封顶）。
- **局内设置**：仅音量滑块 + **「！本局信息」**（点击展开本局倍数/连庄、基本分、番数封顶、
  规则开关状态、AI 强度）；对局中不允许改规则。

### 主界面设置项（目标）

AI 强度（入门/普通/高手）、允许捉冲（点炮胡）、有财神必须爆头、允许吃三摊（承包）。

> 当前进度：设置层已接入 **AI 强度（入门/普通/高手）**、**允许捉冲 / 有财神必须爆头 / 允许吃三摊** 三个规则开关，
> 以及**起始积分（50/100）**、**基本分（1/2/4/8）**、**番数封顶（16/32/64，默认32）**
> （点击切换、即时持久化）。
> AI 打法：入门＝直接打出摸到的牌；普通＝结构评估（保留对子/搭子、不主动打财神）；高手＝在普通基础上偏好打熟张。

---

## 仓库结构

```
.
├── native/                        C++ / Cocos2d-x 工程（本项目主体）
│   ├── Classes/                   游戏逻辑 + UI
│   │   ├── GameLogic/             胡牌判定 / 番型 / 听牌（规则改造重点）
│   │   ├── GameLogic/Player/      真人 & AI 玩家
│   │   ├── Scene/ UI/ App/        场景、界面、入口
│   │   └── Common/                配置、命令、工具
│   ├── Resources/                 牌桌素材（含 UI.ccs 等）
│   ├── cocos2d/                   Cocos2d-x 3.17 引擎源码
│   ├── cocosstudio/               CocoStudio 运行时
│   └── proj.android/              Android 打包工程（Gradle + ndk-build）
├── .github/workflows/android.yml  CI：自动构建 APK 并发布 Release
├── RULES.md                       萧山麻将规则文档
└── LICENSE                        MIT 许可证
```

## 构建

```bash
# 需要：JDK 17、Android SDK(35)、NDK 21.4.7075529，并设置 ANDROID_SDK_ROOT
cd native/proj.android
gradle assembleDebug        # 产物：app/build/outputs/apk/debug/app-debug.apk
```

也可由 GitHub Actions 自动完成：推送到 `master` → 构建 debug APK → 附加到 Release
（产物名 `xiaoshan-mahjong.apk`）。

## 规则文档

萧山麻将的完整规则整理见 [RULES.md](RULES.md)（财神、番型倍数、承包、庄家与连庄等）。

## 许可证

本项目以 [MIT License](LICENSE) 开源。
