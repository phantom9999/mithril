# 简介
携程开源的远程配置服务apollo的c++客户端。

功能:
- 配置自动反序列化
- 配置更新侦测
- 容灾

# 使用
使用流程包括，配置注册、客户端初始化、配置使用、客户端退出等。

## 配置注册
配置注册类似gflags中的宏，分为头文件声明和源文件定义。
properties格式要求value的内容是json格式字符串。
properties类型配置支持分级发布，其他类型数据不支持分级发布，迁移使用前者。

properties格式数据申明和定义:

```
// Prop1.Key1配置的内容:
{
    "key1": "xxxx"
}
// Prop1.Key2配置的内容:
{
    "key2": "xx"
}
// Prop2.Key3配置的内容:
{
    "key3": "cccc"
}
```

配置对应proto的定义如下:

```protobuf
syntax = "proto3";

package example;

message Msg1 {
  string key1 = 1;
}

message Msg2 {
  string key2 = 1;
}

message Msg3 {
  string key3 = 1;
}
```

对应类型注册如下:

```c++
// 申明的代码:
APOLLO_DECLARE(Prop1, Key1, example::Msg1)
APOLLO_DECLARE(Prop1, Key2, example::Msg2)
APOLLO_DECLARE(Prop2, Key3, example::Msg3)

// 定义的代码:
APOLLO_DEFINE(Prop1, Key1, example::Msg1)
APOLLO_DEFINE(Prop1, Key2, example::Msg2)
APOLLO_DEFINE(Prop2, Key3, example::Msg3)
```

配置使用代码如下:

```c++
boost::shared_ptr<example::Msg1> data1 = apollo::GetProp1Key1();
boost::shared_ptr<example::Msg2> data2 = apollo::GetProp1Key2();
boost::shared_ptr<example::Msg3> data3 = apollo::GetProp2Key3();
boost::shared_ptr<example::Msg2> data4 = apollo::GetMsg<example::Msg1>("Prop1", "Key2");

// GetProp1Key1、GetProp1Key2、GetProp2Key3三个函数是注册时通过宏生成的
// GetMsg<example::Msg1>(ns, key)是客户端提供的，用于根据字符串获取对应的配置。
```

其他格式的配置不支持protobuf反序列化，通过namespace直接获取string。
配置注册及使用:

```c++
// 配置申明
APOLLO_DECLARE_JSON(Other1)
// 配置定义
APOLLO_DEFINE_JSON(Other1)
// 配置使用
boost::shared_ptr<std::string> data = apollo::GetOther(apollo::kOther1);
```

## 客户端初始化与退出
客户端初始化和退出直接调用单例的Init和Close函数即可:

```c++
// 初始化
apollo::ApolloConfig::Get().Init();
// 退出
apollo::ApolloConfig::Get().Close();
```

apollo本身是远程配置，客户端的配置自然“从简”，基于gflags:

- apollo_interval 定时更新间隔
- apollo_server apollo api的服务端地址
- apollo_app apollo中的app名
- apollo_cluster apollo中的cluster名
- apollo_tokens apollo的验证tokens
- apollo_timeout 请求apollo api的超时时间

## 安装
依赖:
- brpc
- glog
- gflags
- abseil
- boost

编译安装:

```shell
mkdir build
cd build
cmake ..
make && make install
```

使用:

```cmake
find_package(apollo-cpp)
target_link_libraries(xxx apollo-cpp)
```

## 其他
### 更新问题
apollo没有中间者，需要客户端轮询更新。
本客户端是使用了不带cache的接口去查询，集群多了可能扛不住，虽然目前没有发现这个问题。

### 容灾
api服务的服务稳定性并不是100%，总归会有问题。
当api服务奔溃时，客户需要保障新启动的服务能正常使用配置。

客户端提供了两种容灾手段：

- 配置本地dump
  - 每次配置更新时，自动落盘到本地
  - 新实例启动并且api服务奔溃时，加载较为新鲜的本地配置
  - 涉及配置:
    - apollo_use_sink 是否开启本地落盘
    - apollo_sink_dir 落盘目录
    - apollo_sink_timeout 新鲜度
- 配置集群传递
  - 初始配置从集群其他实例获取
  - 当实例启动时，如果api奔溃，则从本集群其他实例中获取最新配置
  - 要求服务注册"转递"service
  - 涉及配置：
    - apollo_use_deliver 是否开启传递
    - apollo_deliver_server 当前集群





