本开源项目来源于b站恋恋风辰zack

### 项目描述  
这是一个全栈的即时通讯项目，前端基于QT实现气泡聊天对话框，通过QListWidget实现好友列表，利用GridLayout和QPainter封装气泡聊天框组件，基于QT network模块封装http和tcp服务。支持添加好友，好友通信，聊天记录展示等功能，仿微信布局并使用qss优化界面

后端采用分布式设计，分为GateServer网关服务，多个ChatServer聊天服务，StatusServer状态服务以及VerifyServer验证服务。

各服务通过grpc通信，支持断线重连。GateServer网关对外采用http服务，负责处理用户登录和注册功能。登录时GateServer从StatusServer查询聊天服务达到负载均衡，ChatServer聊天服务采用asio实现tcp可靠长链接异步通信和转发, 采用多线程模式封装iocontext池提升并发性能。数据存储采用mysql服务，并基于mysqlconnector库封装连接池，同时封装redis连接池处理缓存数据，以及grpc连接池保证多服务并发访问。

经测试单服务器支持8000连接，多服务器分布部署可支持1W~2W活跃用户。

### 技术点

asio 网络库，grpc，Node.js，多线程，Redis, MySql，Qt 信号槽，网络编程，设计模式

### 项目意义

关于项目意义可结合自身讨论，比如项目解决了高并发场景下单个服务连接数吃紧的情况，提升了自己对并发和异步的认知和处理能力等。
