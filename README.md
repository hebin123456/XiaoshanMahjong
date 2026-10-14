# 萧山麻将 · XiaoshanMahjong

**安卓原生 App**：单机版萧山麻将，**你 vs 3 个电脑 AI**，横屏对局。

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
- [x] CI 出包验证（双架构 APK 可安装于 64 位机型）。
- [ ] 萧山规则适配（C++ 逻辑层，见下）。
  - [x] **财神万能**：白板（z7 / 0x37）可替代任意牌组成顺/刻/将（`GameLogic::analyseCardJoker`），含财神七对亦可判定。
  - [x] 财神打出限制（打出白板时其余三家不能吃/碰/杠，仍可胡）。
  - [ ] 有财神必须爆头（开关 + 番型）。
  - [ ] 允许捉冲（点炮胡）开关。
  - [ ] 允许吃三摊（承包）开关 + 结算。
  - [ ] 抢庄制、吃张打张校验。
  - [ ] 番型倍数表（爆头/碰碰胡/七对/清一色/天胡地胡…）。
    - [x] 清一色判定忽略财神花色（含财神的清一色仍算清一色）。

---

## 萧山麻将规则要点（待适配到 C++ 逻辑层）

| 规则 | 说明 | 目标位置 |
| --- | --- | --- |
| 财神（万能百搭） | **白板（z7）为万能财神**，可替代任意牌组成顺/刻/将；开局翻出的牌仅作「本局财神」展示 | `GameLogic` 胡牌/听牌分析 |
| 财神打出限制 | 白板打出时，其余三家不能吃 / 碰 | `GameEngine` 动作裁决 |
| 有财神必须爆头 | 开关项：开启时手中财神至少留 1 张做将牌（爆头 ×2） | 规则开关 + 番型 |
| 允许捉冲（点炮胡） | 开关项：关闭即「自摸专属」，只能自摸 | `GameLogic::analyseHuCard` |
| 允许吃三摊（承包） | 开关项：关闭时每家最多吃/碰/杠 2 摊；开启时第 3 摊触发**承包**绑定，绑定方独付并加倍 | `GameEngine` 结算 |
| 抢庄制 | 谁胡谁做庄，流局连庄 | `GameEngine` 局末 |
| 吃张打张 | 已吃上家 2 摊后不允许吃张打张 | `GameEngine` 出牌校验 |

### 番型与倍数（目标）

自摸 ×1（基础）｜爆头 ×2｜碰碰胡 ×2｜全倒 ×2｜明杠开 ×2｜暗杠开 ×4｜
财飘 ×4｜七对 ×4｜清七对 ×8｜清一色 ×8｜字一色 ×16｜天胡 ×16｜地胡 ×16。
倍数可叠加，庄家 ×2，封顶 64 倍。

### 主界面设置项（目标）

AI 强度（入门/普通/高手）、允许捉冲（点炮胡）、有财神必须爆头、允许吃三摊（承包）。

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
└── .github/workflows/android.yml  CI：自动构建 APK 并发布 Release
```

## 构建

```bash
# 需要：JDK 17、Android SDK(35)、NDK 21.4.7075529，并设置 ANDROID_SDK_ROOT
cd native/proj.android
gradle assembleDebug        # 产物：app/build/outputs/apk/debug/app-debug.apk
```

也可由 GitHub Actions 自动完成：推送到 `master` → 构建 debug APK → 附加到 Release
（产物名 `xiaoshan-mahjong.apk`）。

## 素材与授权

- 上游工程 **cocostudio-mahjong** 已获得作者授权后并入本项目。
- 麻将牌贴图与 UI 素材来自上游工程自带资源。
