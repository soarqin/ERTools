# 艾尔登法环小工具合集
用Lua语言编写

#### 注意事项
* 法环必须绕过小蓝熊运行，目前有两个办法可以快速绕过小蓝熊运行游戏：
  1. (推荐) 在法环的`eldenring.exe`所在目录创建一个`steam_appid.txt`，里面写上`1245620`，然后用`eldenring.exe`启动游戏
  2. 备份并删除`start_protected_game.exe`，把`eldenring.exe`改名为`start_protected_game.exe`

## 直播工具
#### 用法
* 直接使用
  + 运行 `livestreamingtool.bat`，会开始自动寻找运行的游戏进程，自动判断游戏的开关
    - 当游戏开启后会自动寻找对应的内存数据并输出到特定文件
    - 当游戏关闭后会清空输出文件的内容
  + 输出`info.txt`和`bosses.txt`到`output`目录，可以直接用直播工具读取显示(注意部分直播工具可能要选择文本编码为`UTF-8`)
    - `info.txt`: 当前游戏和角色信息，包含游戏时间、周目数、卢恩数和角色等级属性
    - `bosses.txt`: boss击杀进度，全165个boss，会显示总进度和当前所在区域的击杀进度
* 自定义
  + 修改`livestreamingtool`目录下的`_config_.lua`配置一些选项，可以修改输出目录等
  + 修改`livestreamingtool`目录下的`info.lua`和`bosses.lua`来改变输出文件的格式，你可能需要少量lua的编程知识
