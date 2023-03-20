# mithril

# 代码汇总
- 项目类代码
    - apollo-cpp
        - apollo配置平台的cpp客户端 
        - 支持一站式配置反序列化、容灾等功能
    - faiss_server
        - 基于faiss实现的向量检索服务
        - 支持建库、超惨优化
    - strategy_server
        - 基于taskflow实现的策略服务
    - taskflow_on_fiber
        - 基于boost.fiber实现的类taskflow的图引擎
    - flow_engine
        - 基于boost.fiber实现的图引擎服务
    - taskflow_java
        - java版本的图引擎服务
    - material_server(todo)
        - 正排服务实现
- 工具类代码
    - pb_base_conf
        - 基于protobuf text格式实现的配置系统
    - jeprof_in_use
        - 在线服务继承jeprof内存分析工具
    - concurrent_lru_cache
        - 支持并发的lru缓存
    - chromium_base(todo)
        - chromium的工具代码收集
    - hhvm_utils
        - hhvm带的工具箱
    - normal_structs
        - 一些常用的数据结构
- 工具收集
    - cmake_source
        - 基于cmake实现依赖拉去及打平
    - docker_dev
        - docker的编译镜像
    

