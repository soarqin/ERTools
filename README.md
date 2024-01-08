# ERTools
艾尔登法环小工具合集，Lua语言编写

## 直播工具
#### 用法
* 直接使用
  + 运行 `livestreamingtool.bat`
  + 输出info.txt和bosses.txt到output目录，可以直接用直播工具读取显示
    - info.txt: 当前游戏和角色信息，包含游戏时间、周目数、卢恩数和角色等级属性
    - bosses.txt: boss击杀进度，全165个boss，会显示总进度和当前所在区域的击杀进度
* 自定义
  + 修改livestreamingtool目录下的_config_.lua配置一些选项，可以修改输出目录等
  + 修改livestreamingtool目录下的info.lua和bosses.lua来改变输出文件的格式，你可能需要少量lua的编程知识
