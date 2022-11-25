# something-evil——恶意小程序合集
## 键盘记录器
#### KeyLogger_v1.0.cpp
+ 自动隐藏
+ 记录所有常用按键
+ 将键盘记录伪装在图片文件末尾并含有时间戳
+ 自动注册成开机自启（此功能可能会报毒）
+ 可扩展记录鼠标和窗口标题

#### KeyLogger_Client_v2.0.cpp
+ tcp实时发送键盘记录（只能在低延迟低丢包网络环境下使用，没有做任何粘包拆包的处理）
+ tcp支持心跳包，可自动重连
+ 使用低级键盘钩子和消息循环，记录准确
+ 增加实时状态输出，便于调试

#### KeyLogger_Client_v2.1.cpp
+ 在2.0基础上少量优化

#### KeyLogger_Server_v2.0.cpp
+ 同时兼容v2.0和v2.1的服务端
+ 实时从Client获取键盘消息
+ 可记录到日志中，含时间戳

**TODO：**
+ 使用更加隐蔽的记录方式，以避免被杀毒软件查杀
+ 制作日志解析器
+ 使用```libhv```网络库


## 远程命令执行
#### RCE_Client_v1.0.cpp/RCE_Server_v1.0.cpp
+ 客户端为被控端，可一定程度上避免被防火墙拦截（首次运行不会有Windows防火墙的提示）
+ 支持心跳包
+ 使用system()执行命令，并传回进程返回值（不能进行命令交互）

#### RCE_Client_v2.0.cpp/RCE_Server_v2.0.cpp
+ 优化tcp传输
+ 使用popen()执行命令，支持回显（只能得到进程的stdout，得不到stderr）

#### RCE_Client_v2.1.cpp/RCE_Server_v2.1.cpp
+ 在2.0基础上少量优化

#### RCE_Client_v3.0.cpp/RCE_Server_v3.0.cpp
+ 一个失败的版本

#### RCE_Client_v4.0.cpp/RCE_Server_v4.0.cpp
+ 全新的命令执行方式:CreateProcess()创建```cmd/powershell```子进程，直接通过管道与终端交互
+ 自动管理cmd进程，cmd被taskkill后可自动恢复
+ 使用[libhv](https://github.com/ithewei/libhv)网络库，保证传输稳定，有粘包拆包策略，可自动重连
+ Server可捕获Ctrl+C并发给Client，Client发送SIGINT给子进程以模拟正常的进程终止
+ 使用VS2022构建，支持Win11

#### RCE_Client_v4.1.cpp/RCE_Server_v4.1.cpp
+ 修复4.0的部分bug（详见源文件注释）
+ 提高传输速度
+ 解决Client因```taskkill conhost```造成的崩溃

**TODO：**
+ 使用```libhv```的C接口代替C++接口，以增强扩展性（比如考虑直接使用```libhv```提供的心跳包支持）
+ 增加TLS
+ 增加DNS解析，以应对外网使用的情况
+ 支持Linux服务端
+ 模拟真正的SSH终端，传输键盘操作而非实际输入的文本
+ 以Service形式运行，增加隐蔽性
+ 优化进程管理

备注：预计两年之后继续更新
