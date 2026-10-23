# SC2 解码器状态与暂停交接（v93）

本记录区分「源码实现」「RAM 候选实测」「正式内核包」「完整镜像实测」。
编译成功、微码枚举、播放器名称或时间前进均不替代真实像素、PTS、尾帧和 EOS。
本轮按用户要求收敛 MPEG-4/Xvid、VC-1/WMV3、AVS/AVS+、AV1，完成下列边界较小的工作后暂停；没有将其余失败改判为通过。

## 1. 当前结论与数量

- **这 4 个家族都还没有完成全规格、全部用例验收**；不是还缺 4 个从零编写的解码器。
- MPEG-4、AVS/AVS+、AV1 已有受限的真实硬解正向证据；VC-1/WMV3 的完整图像、完成语义和 EOS 仍未通过。
- 按下文固定边界，剩余 **9 个工作包 F01–F09**。这是交接分类，不是 9 个格式、9 个缺陷或完成百分比；F04 含两个保留失败，F05/F07 含多个未覆盖规格。
- 本轮新增：AV1 128×128 superblock；纠正多 tile 完成计数契约；补齐 AVS/AVS+ 四组正式镜像像素、元数据及 EOF 回归。
- 没有新建或部署完整 v93 镜像；生产继续运行已验收的 v92。0075 已进入补丁系列并完成正式内核包构建，不能因此声称它已在生产镜像中。

### 运行、源码与包身份

| 层级 | 精确身份 / 边界 |
| --- | --- |
| 当前完整镜像 | `LibreELEC-SC2.aarch64-12.2-devel-20261023230059-4d96e6c`，Linux 6.18.57 |
| 当前 boot | `183bef13-96d8-4046-b729-5ed3050e515d`；本轮未整机重启 |
| v93 Linux 提交 | `2bd611734c9a746e552aa5e569395cb13e9e52be` |
| LE 导出补丁 | `devices/SC2/patches/linux/0075-meson-sc2-av1-superblocks-and-tile-completion.patch` |
| 实测 clean RAM 模块 SHA-256 | `809d7a9d67c32d50dac2244bf89eb97b1d275d3e4475b3e8e72555adebd5a851` |
| 正式内核包 | 构建退出 0；应用后的两个改动源文件与开发树逐字节一致；包模块未单独上板 |
| 恢复后的生产模块 build-id | `091b89957bdb208b4a23700e3656bddaea505b41`；不是上述 RAM 候选 |

## 2. 本轮实际完成的工作

### 2.1 AV1：128×128 superblock 与多 tile 完成契约

修改 `linux/drivers/media/platform/amlogic/meson-sc2-vdec/meson_sc2_av1.h` 和同目录 `Kconfig`：

1. 移除对 native sequence 中 `USE_128X128_SUPERBLOCK` 的无条件拒绝。
2. SAO block log2 按 sequence controls 选择 6/7；RPM `seq_flags` bit6 必须与 controls 一致，不能以固件解析结果代替调用者权威。
3. 保留真实 `PIC_END` (`0xe0`)、唯一 frame header、输入 DMA 填充上界、MMU 页数、输出 guard 及原有排空/隔离检查。
4. 不再要求 `PIC_END_LCU_COUNT` 恰好等于几何 SB 数。真实 320×240、128×128、2×2 tile 的 `e0` 事件报告 **8**，几何数却是 **6**，旧等式误拒绝；厂商 `vav1.c` 的完成契约是 `PIC_END` 加非零计数。本轮采用该契约，不硬编码 8，不把 EMPTY 当完成。

权威来源：`reference-kernels/linux/drivers/amlogic/media_modules/frame_provider/decoder/vav1/vav1.c` 的 SAO superblock 配置、`seq_flags` bit6 和 `PIC_END_LCU_COUNT` 非零检查。诊断只证明该计数不必等于几何数；不臆造完整固件计数算法。

修复前，原生生产路径拒绝 128×128 输入。第一候选的四 tile 用例失败已保留；临时观察打印确定 validate=0、syntax=0、`f2→f3→e0`、计数 8 后，所有临时打印撤回，重新编译并测试最终 clean 模块。

| 最终 clean 用例 | 原流工具 | 呈现 | 像素比较 | PTS |
| --- | --- | ---: | --- | --- |
| `sb128` | libaom，128×128，普通量化 | 24 | 2,764,800 NV12 字节逐字节相同 | 全序列匹配 |
| `sb128-tiles` | libaom，128×128，2×2 tile | 24 | 2,764,800 字节逐字节相同 | 全序列匹配 |
| `sb128-lossless` | libaom，128×128，base_q_idx=0 / lossless | 24 | 2,764,800 字节逐字节相同 | 全序列匹配 |
| `sb64-tiles` | libaom，64×64，2×2 tile 回归 | 24 | 2,764,800 字节逐字节相同 | 全序列匹配 |

共 **96 次呈现 / 11,059,200 字节**；其中新增 128×128 覆盖为 72 次，64×64 回归为 24 次。参考来自对原始压缩输入的独立 libdav1d 解码；硬件客户端只接受 DRM PRIME，不允许软件像素回退。全部 96 份原始硬件像素已归档取回主机并逐项核对大小和 SHA-256。通过范围是 320×240 Main8 的这些实际编码组合，不扩展为 Main10、4K、全部 tile 布局或 grain。

正式包构建：在 LE 根目录使用既有项目内工具链环境运行 `PROJECT=Amlogic DEVICE=SC2 ARCH=aarch64 scripts/build linux`。构建重新解包并应用整个补丁系列，退出 0。首次直接在未准备的 `linux/` 目录构建因缺 `include/config/auto.conf` 失败；之后 RAM 构建使用已准备的生产内核头，正式包则由 LE 自己完成完整准备。这两种构建不能混称。

证据目录均为项目根目录下 `Work/sc2-all-codecs-20261022/`：

- `gpt-av1-sb128-oracles-v93-rea.json`、`gpt-av1-sb64-tiles-oracle-v93.json`：真实生成命令、工具 trace 和独立参考摘要。
- `gpt-av1-sb128-before-v93-rea.json`：修复前拒绝。
- `gpt-av1-sb128-after-v93-rea.json`、`gpt-av1-sb128-tile-diagnostic-v93-rea.json`：第一候选及计数分歧；不是最终通过候选。
- `gpt-av1-sb128-clean-v93-rea.json`、`gpt-av1-sb128-comparison-v93.json`：最终 clean 实机、逐帧像素和 PTS 比较。
- `gpt-av1-sb128-clean-pixels-v93.tar`：96 份最终原始硬件图像。
- `gpt-av1-sb128-package-v93.json`：完整包构建日志 `artifact://5405`、源文件和包模块 SHA，及未部署完整镜像的边界。

### 2.2 AVS / AVS+：正式镜像正向回归

使用**未替换的生产 `meson_vdec` 与生产 FFmpeg**，没有 LD_PRELOAD、诊断库或 RAM 解码模块。输入是已保留的原始 fixtures；缺失的 P16 timed NUT 仅原样上传至 `/run`，未改编码 payload。

| 用例 | 原始输出 | 独立像素/元数据结果 |
| --- | --- | --- |
| AVS 352×288 I/P/B | 36 图 | 5,474,304 可见 NV12 字节逐字节一致；完整事件元数据与已验证原流向量相同 |
| AVS 1920×1080 I/P/B | 24 图 | 74,649,600 字节逐字节一致；完整事件元数据相同 |
| AVS+ P16 720×576 TFF | 50 图 | 31,104,000 字节与独立原流软件解码逐字节一致；PTS=`3600*n`、time_base=1/90000、隔行/TFF 正确 |
| AVS+ P16 硬件 EI | 100 场率渐进输出 | 原始奇/偶亮度行共 **20,736,000 样本**逐字节一致；PTS=`3600*n`、duration=3600、time_base=1/180000 |

所有四组均真实观察到 `Dequeue EOF: draining=1, cap.done=1`；不是只看客户端退出 0。STREAMOFF/关闭后 legacy decoder refcnt 回到 0。

普通 AVS raw demux 的原始 PTS 顺序包含 `0,96000,48000,192000,144000,…`，time_base=1/1200000；不能通过独立排序时间戳伪造一致。输出按可见宽高剥离 DMA pitch/padding 后比较；P16 native pitch1920，EI pitch768，不把 padding 当像素。EI 只接受保留源亮度行，不声称插值行等同软件 DI，也未接受其 chroma 一致性。

110 张原生图、100 场 EI 和四份事件清单共 214 个文件已归档、下载并逐项验证 SHA。证据：

- `gpt-image-avs-positive-preflight-v93-rea.json`：生产输入和保护断言。
- `gpt-image-avs-positive-v93-rea.json`：四组真实执行、事件、EOF 与资源释放。
- `gpt-image-avs-positive-comparison-v93.json`：完整像素及元数据比较。
- `gpt-image-avs-pixels-v93.tar`：173,834,240 字节；SHA-256 `bc1eba39b1c96846b6ba3e7a43227737799ef082e47f77676c1d2099e1ceb150`。

本轮没有重跑已知 no-slice / issue767 失败，也没有将它们取消。没有把上述 native 客户端回归冒称新的 Kodi 观看验收。

## 3. 四个家族：已实现内容和保留问题

### MPEG-4 Part 2 / Xvid

已实现原生 stateful MULTI、参考/呈现 token、帧率传递、尾参考与 drain。v79 已使用厂商不完整 B6 VOP padding 取得真实硬件 EMPTY：36 图、PTS0–1400ms/40ms、真实 EOF；单 I 尾参考恰好返回一次，115,200 样本最大差 1/255。Kodi 已使用 `ff-mpeg4_v4l2m2m-drm_prime`，集成帧率为 25fps；既有 strace 未展开 S_PARM 结构，不夸称独立读到 1/25。

**未完成**：显示序号 25、26、28、29、31、32 六张晚 B 图仍损坏，最终 run 最大差244。不是缺输出图数或 EOF；旧 v78 的 32 图无 EOF 是历史失败，不能替代 v79 修复后的结论。额外 VLD reset、FIFO enable/WP 顺序实验未修好像素，均已撤回。

所缺判别量：首个分歧 MB 的真实 future-P skip 决定、bit position 和经认证的状态字段映射；不能猜 LMEM 字段、硬改 Q 或重写源流。证据：`gpt-mpeg4-final-v79-{run,pixels}.json`、`gpt-mpeg4-single-final-v79-*`、`gpt-codec-next-discriminators-v76.json`、`gpt-codec-discriminators-v79-summary.json`。

### VC-1 / WMV3

已有 selector9、workspace/canvas、原生提交与厂商直连的诊断路径；**不是已通过的完整解码器**。32 个未改 packet 的既有观察为30次非零 publication、29次 consumer return；标准1024字节 EOS 没有刷出缺图。最后 retained display 也不能解释全部缺失，不能把“寄存器非零”当完整重建。

所缺：真实 parsed/completed-picture 区分、尾参考释放/EOS 权威、原始 PTS/独立像素以及 exact-HK1 安全 selector9 选中成员的身份。包内 GXL/G12A/SM1 payload 与 TL1/TM2 alias 已核定，但 TEE 绕过 normal-world CPU 过滤，不能推断实际选中了 SM1。没有替换微码、绕过 TEE 或对未知指令乱补完成。

证据：`gpt-vc1-factory-handoff.json`、`gpt-wmv3-original-semantics-v76.json`、`gpt-codec-next-discriminators-v76.json`、`gpt-factory-package-members-v82.json`；另见主 README 的 C01/C02/member-binding 段。

### AVS / AVS+

已实现 frame-based AVSM、逐 run 核心/VLD 初始化、decoder-local canvas、BUFFEROUT 呈现身份、PIC_DONE 当前图完成、只释放一个已完成且唯一 token/ANC0 匹配的 I/P 尾参考。AVS 352/1080 的 v70 双参考（独立软件与 xavs encoder reconstruction）、单 I、失败后重开均有既有正向证据。60 图 HD bunny 已由支持加权预测的独立标量参考补齐，82,944,000 样本完全相同；旧2289差异来自旧参考缺工具，不重复跑硬件充数。

P16 原始576i50图、原始 PTS、100场硬件 EI 和真实 Kodi/HDMI 观看已有 v76/v77 证据；本轮进一步补齐正式镜像四组回归，见第2.2节。

**保留失败**：旧720×576 no-slice-start 输入为零 consumer 图；issue767 原流只返回此前三张准确图后失败。v79 四个语法等价诊断变体只返回4图再 drain timeout，无 EOF，不能冒充修复原流。AU5 耗尽与延迟 AU4 的 BUFFEROUT 不能当同一张图完成。缺所选 AVSM 的 parser/reservoir 与当前 REC 完成映射。

**未覆盖规格**：AVS P2 隔行、AVS+1080级原始流与相应独立参考。576i P16 通过不推广到它们。

证据：`gpt-avsm-final-v70-comparison.json`、`gpt-avs-plus-pes-timed-v76-comparison.json`、`gpt-avs-plus-pes-timed-di-v77-comparison.json`、`gpt-avs-bunny-weighted-reference-v79-comparison.json`、`gpt-avs-issue767-{tail-syntax,row-slice-reference}-v79.json`，以及第2.2节新证据。

### AV1

已有原生 Request、native controls 规范化、连续 I/P/B/隐藏参考、show-existing、CDF/segmentation 状态与多 context 隔离的覆盖证据。v89–v91 superresolution 包括 denominator9/16 与同序列9..16动态 coded 宽度；v90 原始 frame-ID 校验包括回绕/redisplay，并拒绝四类坏 ID，不通过规范化抹除源错误。

v92 完整生产镜像已有65张动态 superres/合法ID呈现全等；同一 context 在第30图后 seek/flush，608张/70,041,600字节及完整PTS全等。实际 HDMI 源计数175→前拖584→后拖116→恢复293，证明画面而非 RPC 时钟。第一批菜单截图保留为过早观察，不计通过。v93 新增72张128×128及24张64×64多 tile 回归，仍是 RAM 模块范围，见第2.1节。

**继续明确拒绝**：grain、intrabc、timing model、非 identity global motion，以及当前 Main8 工作区/输出边界外的请求。Main10全位深和4K尚未由本后端接受；原厂声称能力不是本实现验收。

grain 已找到 producer 参数/表和 VPP 消费路径，但缺 SC2 LUTDMA 有限完成、排空或可证明隔离后可释放旧表面的契约。只有 current-slot / disable-trigger 不够，decoder DMC drain 也不替代 display DMA。全部 AFG8/modifier/job/consumer 试作已撤回，无占位脚手架；不能靠等一个 vblank 或固定延时冒险释放。

证据：`gpt-av1-superres-comparison-v89.json`、`gpt-av1-frameid-comparison-v90.json`、`gpt-av1-superres-matrix-comparison-v91.json`、`gpt-av1-film-grain-blocker-v91.json`、`gpt-native-image-av1-{comparison,seek-comparison}-v92.json`，及第2.1节。

## 4. 剩余 9 个工作包与后续顺序

| ID | 未完成交付 | 当前阻塞 / 下一证据 | 结清条件 |
| --- | --- | --- | --- |
| F01 | MPEG-4 六张晚 B 图像素 | 首分歧 MB 的 future-P skip/bit-position 权威缺失；对应 V01 | 原36图保留，六张晚B与独立原流参考通过，PTS/真实EOF不退化 |
| F02 | VC-1/WMV3 完整图像、完成与尾参考/EOS | selector9 图完成/尾参考语义未建立；对应 V02 | 未改32包不缺图，独立像素/PTS/真实EOS及观看通过；SP/MP/AP边界不外推 |
| F03 | VC-1 安全选中成员身份 | 只有候选成员/alias，无实际 secure payload 绑定；对应 V03 | 实际成员与摘要或可追溯等价映射，不靠 normal-world fallback 猜测 |
| F04 | AVS 两个原始兼容失败 | no-slice 与 issue767 的 parser/reservoir/完成映射；对应 V04 | 两个原始输入均完整像素/PTS/EOF通过，不用派生重写流顶替 |
| F05 | AVS P2 隔行、AVS+1080级规格 | 尚无足够原流和独立 oracle；对应 V05 | 原流、场序/PTS、全图像素、EOS、EI/观看按适用规格验收 |
| F06 | AV1 grain | 缺 SC2 LUTDMA 完成/排空/隔离生命周期权威；对应 V06 | grain-correct实际像素/输出及安全表面释放；拒绝保留至契约建立 |
| F07 | AV1 其余工具、位深、4K边界 | intrabc、timing、nonidentity global motion仍拒绝；Main10/4K工作区和全位深路径未通过；对应 V06/V07 | 分工具建立完整权威及原流像素/错误路径，不以当前Main8样本外推 |
| F08 | 0075 完整镜像交付 | 源码、补丁、正式内核包已完成；尚未生成/部署包含0075的新完整镜像 | image/TAR/启动载荷校验、双端备份、预先通知后外置介质更新启动，再验生产128×128及多tile像素 |
| F09 | 四族剩余生产端到端状态转换回归 | V09仅部分通过；AVS四组及既有AV1不重开 | 对剩余适用族补齐同context seek、source-change、缓冲持有、停止重开、错误恢复和实际观看；失败族先满足其像素/EOS前提 |

续作顺序：

1. 用户要求恢复工作后，优先 F08：把已经实测的0075做成完整镜像；不能把当前4d96e6c标为含0075。
2. 推进 F09 中尚未覆盖且可直接复用原流/oracle的状态转换；不重复已经通过的 v92 AV1/本轮 AVS 四组。
3. F07 按可取得完整源码契约的独立工具推进；F05 取得匹配原流/oracle后再验。
4. F01–F04、F06 按缺失权威逐个补证据；不重放没有新判别量的 reset/offset/delay 猜测。

绝对音画同步另缺同一OBS/HDMI采集链路的已知同步源校准，不能从现有延迟直接调盒端补偿；不是以上9项中暗中豁免的一项。eMMC安装、无线连接、CEC、光纤/模拟、TCL兼容及上游交付继续由根 `skill.md` 原ID跟踪，不在本轮重新展开或删除。

## 5. 其他已实现格式的保留记录

以下复用已有实测，不是本轮新增运行；完整历史、版本/输入/证据边界见 [主 README](README.md)。全部已有 native 实现已整合进 v81/v92 源码与镜像，但“已打包”不代表每个旧候选像素用例已重新完成生产回归。

| 格式 | 已实现与既有实测 | 未据此声称完成 |
| --- | --- | --- |
| HEVC Main/Main10 | Request、I/P/B、seek、Kodi；320/1080全位深FBC离线独立解压与原流软解共51,609,600次10bit样本比较全等；真实4K HTSP修复有实际画面 | 10bit HDMI采集、HDR、绝对同步、全部工具/规格 |
| H.264 | 原生MULTI、progressive/MBAFF/PAFF；湖南PAFF24图/74,649,600字节全等；真实CCTV1/CCTV13及硬件EI50Hz画面已观察 | 全规格、全部source-change/坏流/生产回归；不能将旧错误MMCO样例改判为通过 |
| VP9 P0/P2 | 原生实现；既有每profile36图与独立全位深样本匹配 | 完整生产端到端规格/状态转换矩阵 |
| AVS2 | 原生实现；640×360 8bit32图和4K10bit预期23次呈现，独立全位深样本含低位匹配 | 全规格/全部生产回归 |
| MPEG-1/2 | MPEG1 36图/PTS/渐进/EOF；MPEG2 1080i24图、48场EI与Kodi实画；最大IDCT样本差4/255已记录 | 不冒称字节全等，不以插值行替代源行证据 |
| MJPEG video | 原生progressive36图、paired-field12图比较最大差1/255；v78另有36图/EOF与START-after-EOS烟测 | 全部still/progressive-JPEG变体和吞吐上限；JPEG unlimited不是分配保证 |

H.263、H.264 MVC、RealVideo硬件路径仍待 exact-HK1 核定；VP8/VP6/AVS3/VVC/12bit/422/444不因generic源码或XML条目新增为已支持。原厂TA/video_ucode未更换，公开分发许可仍是单独门槛。

## 6. 暂停时已恢复的状态

- 全部实验后恢复原生产模块并核对 build-id；native/legacy 两个 decoder refcnt均为0。
- Kodi、hk1-panel、sc2-console active/running；sc2-optee-preload active/exited；四者 NRestarts=0、ExecMainStatus=0，systemd running且无failed。
- `/flash` 为只读；eMMC `mmcblk2`、`mmcblk2boot0`、`mmcblk2boot1`均只读；本轮未写启动载荷、未安装eMMC、未替换安全固件。
- 本轮384项自建RAM文件/目录已清理；实际像素、oracle、完整REA和摘要留在主机Work中，不入Git、不删除旧失败证据。
- 第一次收尾查询误用了不存在的 `serial-getty@ttyAML0` / `sc2-video-firmware-preload` 名称；不能由该查询推断真实服务停机。已按既有真实名称补采 `LoadState/ActiveState`，详见 `gpt-codec-pause-real-units-v93-rea.json`。
- 最终证据：`gpt-codec-pause-cleanup-v93-rea.json`、`gpt-codec-pause-real-units-v93-rea.json`、`gpt-codec-pause-state-v93.json`。后者保留错误名称查询并单列实际服务结果，不覆盖原始观察。

**按用户要求在这里暂停**。后续清单、失败样本、回退备份及未完成验收均保留；暂停不是项目完成或全部格式通过。
